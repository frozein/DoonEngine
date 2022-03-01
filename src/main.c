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
GLuint SCREEN_W = 1280 * 4;
GLuint SCREEN_H = 720 * 4;
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
	//init GLFW:
	//---------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	//create and init window:
	//---------------------------------
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	GLFWwindow* window = glfwCreateWindow(SCREEN_W / 4, SCREEN_H / 4, "VoxelEngine", NULL, NULL);
	if(window == NULL)
	{
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	//load opengl functions:
	//---------------------------------
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD\n");
		return -1;
	}

	//set gl viewport:
	//---------------------------------
	glViewport(0, 0, SCREEN_W / 4, SCREEN_H / 4);

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
		return -1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER");
		glDeleteVertexArrays(1, &quadBuffer);
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
	if(!init_voxel_pipeline((uvec2){SCREEN_W, SCREEN_H}, finalTex, (uvec3){3, 3, 3}, 11, (uvec3){3, 3, 3}, 11, 11))
	{
		ERROR_LOG("ERROR - FAILED TO INTIALIZE VOXEL PIPELINE\n");
		return -1;
	}

	//generate voxel data:
	//---------------------------------
	for(int x = 0; x < CHUNK_SIZE_X; x++)
		for(int y = 0; y < CHUNK_SIZE_Y; y++)
			for(int z = 0; z < CHUNK_SIZE_Z; z++)
			{
				//cylinder:
				for(int i = 0; i < 2; i++)
				{
					Voxel vox;

					float distance = vec2_distance((vec2){4, 4}, (vec2){x, z});
					vox.material = (distance < 4.0f) ? 0 : 255;
					vox.albedo.x = pow(x / 8.0f, GAMMA);
					vox.albedo.y = pow(y / 8.0f, GAMMA);
					vox.albedo.z = pow(z / 8.0f, GAMMA);

					voxelChunks[i].voxels[x][y][z] = voxel_to_voxelGPU(vox);
				}

				//block:
				for(int i = 2; i < 11; i++)
				{
					Voxel vox;

					vox.material = 1;
					vox.albedo = (vec3){pow(0.8588f, GAMMA), pow(0.7922f, GAMMA), pow(0.6118f, GAMMA)};

					voxelChunks[i].voxels[x][y][z] = voxel_to_voxelGPU(vox);
				}
			}

	//--------------//

	for(int x = 0; x < map_size().x; x++)
		for(int y = 0; y < map_size().y; y++)
			for(int z = 0; z < map_size().z; z++)
			{
				if(y == 0)
					voxelMap[x + map_size().x * y + map_size().x * map_size().y * z] = 2 + x + z * 3;
				else if(x == 1 && z == 1)
					voxelMap[x + map_size().x * y + map_size().x * map_size().y * z] = y - 1;
				else
					voxelMap[x + map_size().x * y + map_size().x * map_size().y * z] = -1;
			}

	//--------------//

	voxelLightingRequests[0]  = (ivec4){0, 0, 0};
	voxelLightingRequests[1]  = (ivec4){0, 0, 1};
	voxelLightingRequests[2]  = (ivec4){0, 0, 2};
	voxelLightingRequests[3]  = (ivec4){1, 0, 0};
	voxelLightingRequests[4]  = (ivec4){1, 0, 1};
	voxelLightingRequests[5]  = (ivec4){1, 0, 2};
	voxelLightingRequests[6]  = (ivec4){2, 0, 0};
	voxelLightingRequests[7]  = (ivec4){2, 0, 1};
	voxelLightingRequests[8]  = (ivec4){2, 0, 2};
	voxelLightingRequests[9]  = (ivec4){1, 1, 1};
	voxelLightingRequests[10] = (ivec4){1, 2, 1};

	//send data to GPU (TEMPORARY):
	//---------------------------------
	send_all_data_temp();

	//calculate indirect lighting:
	//---------------------------------
	for(int i = 0; i < 300; i++)
	{
		update_voxel_indirect_lighting(11, glfwGetTime());
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
			printf("%f\n", 1 / (cumTime / numFrames));
			numFrames = 0;
			cumTime = 0.0f;
		}

		//process keyboard input:
		process_input(window);

		//update cam direction:
		mat3 rotate = mat4_to_mat3(mat4_rotate_euler(MAT4_IDENTITY, (vec3){pitch, yaw, 0.0f}));

		camFront       = mat3_mult_vec3(rotate, (vec3){ 0.0f, 0.0f, fov });
		vec3 camPlaneU = mat3_mult_vec3(rotate, (vec3){-1.0f, 0.0f, 0.0f});
		vec3 camPlaneV = mat3_mult_vec3(rotate, (vec3){ 0.0f, 1.0f * ASPECT_RATIO, 0.0f});

		//render voxels:
		update_voxel_direct_lighting(11, camPos);
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