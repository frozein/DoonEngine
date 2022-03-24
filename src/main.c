#define GLFW_DLL

#include "DoonEngine/math/all.h"
#include "DoonEngine/shader.h"
#include "DoonEngine/texture.h"
#include "DoonEngine/voxel.h"
#include "DoonEngine/globals.h"
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//Processes keyboard input for the current frame. CALL ONCE PER FRAME ONLY
void process_input(GLFWwindow *window);

//Handles updates to the mouse position. DO NOT CALL DIRECTLY
void mouse_callback(GLFWwindow* window, double x, double y);
//Handles scroll wheel movement. DO NOT CALL DIRECTLY
void scroll_callback(GLFWwindow* window, double offsetX, double offsetY);
//Handles window resizeing. DO NOT CALL DIRECTLY
void framebuffer_size_callback(GLFWwindow* window, int w, int h);

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  ERROR_LOG("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

//--------------------------------------------------------------------------------------------------------------------------------//

//screen dimensions:
GLuint SCREEN_W = 1920;
GLuint SCREEN_H = 1080;
GLfloat ASPECT_RATIO = 9.0 / 16.0;

//cam stuff:
vec3 camPos =      {0.0f, 0.0f,  0.0f};
vec3 camFront =    {0.0f, 0.0f,  1.0f};
const vec3 camUp = {0.0f, 1.0f,  0.0f};

float pitch = 0.0f;
float yaw = 0.0f;
float lastX = 400.0f, lastY = 300.0f;
bool firstMouse = true;
float fov = 0.8f;

float deltaTime = 0.0f;

#define GAMMA 2.2f

//--------------------------------------------------------------------------------------------------------------------------------//

int main()
{
	int error;

	//init GLFW:
	//---------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	//create and init window:
	//---------------------------------
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "VoxelEngine", NULL, NULL);
	if(window == NULL)
	{
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		scanf("%d", &error);
		return -1;
	}
	glfwMakeContextCurrent(window);

	//load opengl functions:
	//---------------------------------
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD\n");
		scanf("%d", &error);
		return -1;
	}

	//set gl viewport:
	//---------------------------------
	glViewport(0, 0, SCREEN_W, SCREEN_H);

	glEnable              ( GL_DEBUG_OUTPUT );
	glDebugMessageCallback( MessageCallback, 0 );

	//set callback funcs:
	//---------------------------------
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback); 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//generate shader program:
	//---------------------------------
	int quadShader = shader_program_load("shaders/quad.vert", NULL, "shaders/quad.frag", NULL);
	if(quadShader < 0)
	{
		shader_program_free(quadShader);
		glfwTerminate();
		scanf("%d", &error);

		return -1;
	}

	//generate quad buffer:
	//---------------------------------
	GLfloat quadVertices[] = 
	{
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
	};
	GLuint quadIndices[] = 
	{
		0, 1, 3,
		1, 2, 3
	};

	unsigned int quadBuffer, VBO, EBO;
	glGenVertexArrays(1, &quadBuffer);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(quadBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER");
		glDeleteVertexArrays(1, &quadBuffer);
		scanf("%d", &error);
		return -1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER");
		glDeleteVertexArrays(1, &quadBuffer);
		scanf("%d", &error);
		return -1;
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(long long)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(long long)(3 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(1);

	//generate texture:
	//---------------------------------
	Texture finalTex = texture_load_raw(GL_TEXTURE_2D, SCREEN_W, SCREEN_H, GL_RGBA, NULL, false);
	texture_param_scale(finalTex, GL_LINEAR, GL_LINEAR);
	texture_param_wrap(finalTex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

	//initialize voxel pipeline:
	//---------------------------------
	if(!init_voxel_pipeline((uvec2){SCREEN_W, SCREEN_H}, finalTex, (uvec3){10, 3, 10}, 300, (uvec3){10, 3, 10}, 300, 300))
	{
		ERROR_LOG("ERROR - FAILED TO INTIALIZE VOXEL PIPELINE\n");
		scanf("%d", &error);
		return -1;
	}

	//generate voxel data:
	//---------------------------------
	for(int x = 0; x < 10; x++)
		for(int y = 0; y < 3; y++)
			for(int z = 0; z < 10; z++)
			{
				int index = FLATTEN_INDEX(x, y, z, voxel_map_size());

				voxelMap[index] = index;
				voxelLightingRequests[index] = (ivec4){x, y, z};
			}

	for(int x = 0; x < CHUNK_SIZE_X * 10; x++)
		for(int y = 0; y < CHUNK_SIZE_Y * 3; y++)
			for(int z = 0; z < CHUNK_SIZE_Z *  10; z++)
			{
				Voxel vox;
				int index = FLATTEN_INDEX(x / 8, y / 8, z / 8, voxel_map_size());

				if(y == 6 || y == 7)
				{
					vox.material = 0;
					vox.albedo = (vec3){pow(0.8588f, GAMMA), pow(0.7922f, GAMMA), pow(0.6118f, GAMMA)};
					vox.normal = y == 7 ? (vec3){0.0, 1.0, 0.0} : (vec3){0.0, -1.0, 0.0};
				}
				else if(x >= 78 && y > 7)
				{
					vox.material = 1;
					vox.albedo = (vec3){pow(0.9f, GAMMA), pow(0.9f, GAMMA), pow(0.9f, GAMMA)};
					vox.normal = (vec3){-1.0, 0.0, 0.0};
				}
				else if(z >= 78 && y > 7)
				{
					vox.material = 1;
					vox.albedo = (vec3){pow(0.9f, GAMMA), pow(0.9f, GAMMA), pow(0.9f, GAMMA)};
					vox.normal = (vec3){0.0, 0.0, -1.0};
				}
				else
					vox.material = 255;


				voxelChunks[voxelMap[index]].voxels[x % 8][y % 8][z % 8] = voxel_to_voxelGPU(vox);
			}

	uvec3 spherePositions[8] = {(uvec3){10, 14, 12}, (uvec3){11, 14, 35}, (uvec3){32, 14, 17}, (uvec3){55, 14, 13}, (uvec3){53, 14, 34}, (uvec3){28, 14, 45}, (uvec3){20, 14, 65}, (uvec3){55, 14, 60}};
	vec3 sphereColors[8] = {(vec3){0.0f, 0.0f, 0.0f}, (vec3){0.1f, 0.1f, 1.0f}, (vec3){0.224f, 0.831f, 0.718f}, (vec3){1.0f, 1.0f, 1.0f}, (vec3){0.569f, 0.224f, 0.831f}, (vec3){0.839f, 0.235f, 0.255f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.306f, 0.831f, 0.224f}};
	unsigned int sphereMaterials[8] = {0, 3, 2, 2, 4, 4, 2, 0};

	for(int i = 0; i < 8; i++)
	{
		uvec3 pos = spherePositions[i];

		for(int x = pos.x - 6; x <= pos.x + 6; x++)
			for(int y = pos.y - 6; y <= pos.y + 6; y++)
				for(int z = pos.z - 6; z <= pos.z + 6; z++)
				{
					Voxel vox;
					int index = FLATTEN_INDEX(x / 8, y / 8, z / 8, voxel_map_size());

					float distance = vec3_distance((vec3){pos.x, pos.y, pos.z}, (vec3){x, y, z});
					vox.material = (distance < 7.0f) ? sphereMaterials[i] : 255;
					vox.normal = (vec3){x - (int)pos.x, y - (int)pos.y, z - (int)pos.z};

					float maxNormal = abs(vox.normal.x);
					if(abs(vox.normal.y) > maxNormal)
						maxNormal = abs(vox.normal.y);
					if(abs(vox.normal.z) > maxNormal)
						maxNormal = abs(vox.normal.z);

					vox.normal = vec3_scale(vox.normal, 1 / maxNormal);

					if(sphereColors[i].x == 0.0f)
					{
						vox.albedo = (vec3){pow(x / (pos.x + 6.0f), GAMMA), pow(y / (pos.y + 6.0f), GAMMA), pow(z / (pos.z + 6.0f), GAMMA)};
					}
					else
					{
						vox.albedo = (vec3){pow(sphereColors[i].x, GAMMA), pow(sphereColors[i].y, GAMMA), pow(sphereColors[i].z, GAMMA)};
					}

					voxelChunks[voxelMap[index]].voxels[x % 8][y % 8][z % 8] = voxel_to_voxelGPU(vox);
				}
	}

	//--------------//

	voxelMaterials[0].emissive = false;
	voxelMaterials[0].specular = 0.0f;
	voxelMaterials[0].opacity = 1.0f;

	voxelMaterials[1].emissive = false;
	voxelMaterials[1].specular = 1.0f;
	voxelMaterials[1].opacity = 1.0f;
	voxelMaterials[1].reflectSky = true;
	voxelMaterials[1].shininess = 100;

	voxelMaterials[2].emissive = true;
	voxelMaterials[2].specular = 0.0f;
	voxelMaterials[2].opacity = 1.0f;

	voxelMaterials[3].emissive = false;
	voxelMaterials[3].specular = 0.7f;
	voxelMaterials[3].opacity = 1.0f;
	voxelMaterials[3].reflectSky = false;
	voxelMaterials[3].shininess = 3;

	voxelMaterials[4].emissive = false;
	voxelMaterials[4].specular = 0.7f;
	voxelMaterials[4].opacity = 0.5f;
	voxelMaterials[4].reflectSky = false;
	voxelMaterials[4].shininess = 10;

	//send data to GPU (TEMPORARY):
	//---------------------------------
	send_all_data_temp();

	//calculate indirect lighting:
	//---------------------------------
	//sunStrength = (vec3){0.0f, 0.0f, 0.0f}; //for night:
	//sunDir = vec3_normalize((vec3){-0.2f, 1.0f, -0.2f});
	//sunStrength = (vec3){0.5f, 0.3f, 0.15f}; //for sunset:
	//sunDir = vec3_normalize((vec3){-1.0f, 0.7f, -1.0f});
	for(int i = 0; i < 3000; i++)
	{
		update_voxel_indirect_lighting(10 * 3 * 10, glfwGetTime());
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	}

	//main loop:
	//---------------------------------
	float lastFrame = glfwGetTime();
	int numFrames = 0;
	float cumTime = 0.0;

	while(!glfwWindowShouldClose(window))
	{
		//find deltatime:
		numFrames++;
		float currentTime = glfwGetTime();
		deltaTime = currentTime - lastFrame;
		cumTime += deltaTime;
		lastFrame = currentTime;

		if(cumTime >= 1.0f)
		{
			printf("AVG. FPS: %f\n", 1 / (cumTime / numFrames));
			numFrames = 0;
			cumTime = 0.0f;
		}

		//process keyboard input:
		process_input(window);

		//update cam direction:
		/*camPos.x = sin(glfwGetTime() / 3.0f) * 7.0f + 5.0f;
		camPos.y = 5.0f;
		camPos.z = cos(glfwGetTime() / 3.0f) * 7.0f + 5.0f;

		yaw = glfwGetTime() / 3.0f * RAD_TO_DEG + 180.0f;
		pitch = 35.0f;*/

		mat3 rotate = mat4_to_mat3(mat4_rotate_euler(MAT4_IDENTITY, (vec3){pitch, yaw, 0.0f}));

		camFront       = mat3_mult_vec3(rotate, (vec3){ 0.0f, 0.0f, fov });
		vec3 camPlaneU = mat3_mult_vec3(rotate, (vec3){-1.0f, 0.0f, 0.0f});
		vec3 camPlaneV = mat3_mult_vec3(rotate, (vec3){ 0.0f, 1.0f * ASPECT_RATIO, 0.0f});

		//render voxels:
		update_voxel_direct_lighting(10 * 3 * 10, camPos);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
		draw_voxels(camPos, camFront, camPlaneU, camPlaneV);

		texture_activate(finalTex, 0);
		shader_program_activate(quadShader);


		glBindVertexArray(quadBuffer);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));

		//finish rendering and swap:
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up and close:
	//---------------------------------
	deinit_voxel_pipeline();
	glDeleteVertexArrays(1, &quadBuffer);
	shader_program_free(quadShader);
	glfwTerminate();

	scanf("%d", &error);
	return 0;
}

void mouse_callback(GLFWwindow *window, double x, double y)
{
	if(firstMouse)
	{
		lastX = x;
        lastY = y;
        firstMouse = false;
	}

	float offsetX = x - lastX;
	float offsetY = lastY - y;
	lastX = x;
	lastY = y;

	const float sens = 0.25f;
	offsetX *= sens;
	offsetY *= sens;

	yaw -= offsetX;
	pitch -= offsetY;

	if(pitch > 89.0f)
		pitch = 89.0f;
	if(pitch < -89.0f)
		pitch = -89.0f;
}

void scroll_callback(GLFWwindow* window, double offsetX, double offsetY)
{
    fov += (float)offsetY / 10.0f;
    if (fov < 0.5f)
        fov = 0.5f;
    if (fov > 2.0f)
        fov = 2.0f; 
}

void process_input(GLFWwindow *window)
{
	const float camSpeed = 3.0f * deltaTime;

	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		viewMode = 0;
	if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		viewMode = 1;
	if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		viewMode = 2;
	if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		viewMode = 3;
	if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		viewMode = 4;
	if(glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		viewMode = 5;

	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camPos = vec3_add(camPos, vec3_scale(vec3_normalize((vec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camPos = vec3_subtract(camPos, vec3_scale(vec3_normalize((vec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camPos = vec3_subtract(camPos, vec3_scale(vec3_normalize(vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camPos = vec3_add(camPos, vec3_scale(vec3_normalize(vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camPos.y += camSpeed; 
	if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camPos.y -= camSpeed;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, w, h);
}