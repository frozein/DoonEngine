#define GLFW_DLL

#include "DoonEngine/math/all.h"
#include "DoonEngine/utility/shader.h"
#include "DoonEngine/voxel.h"
#include "DoonEngine/voxelShapes.h"
#include "DoonEngine/globals.h"
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GLAD/glad.h>
#include <GLFW/glfw3.h>
#include <STB/stb_image.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//Processes keyboard input for the current frame. CALL ONCE PER FRAME ONLY
void process_input(GLFWwindow *window);

//Handles updates to the mouse position. DO NOT CALL DIRECTLY
void mouse_pos_callback(GLFWwindow* window, double x, double y);
//Handles updates to the mouse buttons. DO NOT CALL DIRECTLY
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
//Handles scroll wheel movement. DO NOT CALL DIRECTLY
void scroll_callback(GLFWwindow* window, double offsetX, double offsetY);
//Handles window resizeing. DO NOT CALL DIRECTLY
void framebuffer_size_callback(GLFWwindow* window, int w, int h);
//Prints any OpenGL error or warning messages to the screen
void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

//--------------------------------------------------------------------------------------------------------------------------------//

//map:
DNmap* activeMap;
DNmap* treeMap;
DNmap* sphereMap;

//screen dimensions:
GLuint SCREEN_W = 1920;
GLuint SCREEN_H = 1080;
GLfloat ASPECT_RATIO = 9.0 / 16.0;

//cam stuff:
DNvec3 camPos =      {0.0f, 0.0f,  0.0f};
DNvec3 camFront =    {0.0f, 0.0f,  1.0f};
const DNvec3 camUp = {0.0f, 1.0f,  0.0f};
unsigned int viewMode = 0;

float pitch = 0.0f;
float yaw = 0.0f;
float fov = 0.8f;

float lastX = 400.0f, lastY = 300.0f;
bool firstMouse = true;

float deltaTime = 0.0f;

bool updateData = true;

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
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	//create and init window:
	//---------------------------------
	GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "VoxelEngine", NULL, NULL);
	if(window == NULL)
	{
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		scanf("%d", &error);
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

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
	glDebugMessageCallback( message_callback, 0 );

	//set callback funcs:
	//---------------------------------
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback); 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//generate shader program:
	//---------------------------------
	int quadShader = DN_program_load("shaders/quad.vert", NULL, "shaders/quad.frag", NULL);
	if(quadShader < 0)
	{
		DN_ERROR_LOG("quad shader failed\n");
		DN_program_free(quadShader);
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
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER");
		glDeleteVertexArrays(1, &quadBuffer);
		scanf("%d", &error);
		return -1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER");
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
	GLuint finalTexture;
	glGenTextures(1, &finalTexture);
	glBindTexture(GL_TEXTURE_2D, finalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_W, SCREEN_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//initialize voxel pipeline:
	//---------------------------------
	if(!DN_init())
	{
		DN_ERROR_LOG("ERROR - FAILED TO INTIALIZE VOXEL PIPELINE\n");
		glfwTerminate();
		scanf("%d", &error);
		return -1;
	}

	treeMap = DN_create_map((DNuvec3){5, 5, 5}, (DNuvec2){SCREEN_W, SCREEN_H}, false, 0);
	treeMap->sunDir = (DNvec3){-1.0f, 1.0f, -1.0f};

	sphereMap = DN_create_map((DNuvec3){30, 30, 30}, (DNuvec2){SCREEN_W, SCREEN_H}, true, 1024);
	sphereMap->sunDir = (DNvec3){-1.0f, 1.0f, -1.0f};
	activeMap = sphereMap;

	//load model:
	//---------------------------------
	DNvoxelModel model;
	DN_load_vox_file("tree.vox", &model);
	DN_calculate_model_normals(2, &model);
	DN_place_model_into_world(treeMap, model, (DNivec3){0, 0, 0});

	//generate voxel data (for testing with sphere):
	//---------------------------------
	for(int z = 0; z < sphereMap->mapSize.z * DN_CHUNK_SIZE.z; z++)
		for(int y = 0; y < sphereMap->mapSize.y * DN_CHUNK_SIZE.y; y++)
			for(int x = 0; x < sphereMap->mapSize.x * DN_CHUNK_SIZE.x; x++)
			{
				if(DN_vec3_distance((DNvec3){x, y, z}, (DNvec3){116.0f, 116.0f, 116.0f}) < 116.0f)
				{
					DNvoxel vox;
					vox.material = 0;
					vox.normal = (DNvec3){x - 116.0f, y - 116.0f, z - 116.0f};

					float maxNormal = fabs(vox.normal.x);
					if(fabs(vox.normal.y) > maxNormal)
						maxNormal = fabs(vox.normal.y);
					if(fabs(vox.normal.z) > maxNormal)
						maxNormal = fabs(vox.normal.z);

					vox.normal = DN_vec3_scale(vox.normal, 1.0f / maxNormal);

					DNivec3 chunkPos;
					DNivec3 localPos;
					DN_separate_position((DNivec3){x, y, z}, &chunkPos, &localPos);
					DN_set_voxel(sphereMap, chunkPos, localPos, vox);
				}
			}

	/*for(int z = 0; z < sphereMap->mapSize.z * DN_CHUNK_SIZE.z; z++)
		for(int y = 0; y < sphereMap->mapSize.y * DN_CHUNK_SIZE.y; y++)
			for(int x = 0; x < sphereMap->mapSize.x * DN_CHUNK_SIZE.x; x++)
			{
				DNvoxel vox;

				if(y == 6 || y == 7)
				{
					vox.material = 0;
					vox.normal = y == 7 ? (DNvec3){0.0, 1.0, 0.0} : (DNvec3){0.0, -1.0, 0.0};
				}
				else if(x >= 78 && y > 7)
				{
					vox.material = 1;
					vox.normal = (DNvec3){-1.0, 0.0, 0.0};
				}
				else if(z >= 78 && y > 7)
				{
					vox.material = 1;
					vox.normal = (DNvec3){0.0, 0.0, -1.0};
				}
				else
					continue;

				DN_set_voxel(sphereMap, (DNivec3){x / 8, y / 8, z / 8}, (DNivec3){x % 8, y % 8, z % 8}, vox);
			}

	DNuvec3 spherePositions[8] = {(DNuvec3){10, 14, 12}, (DNuvec3){11, 14, 35}, (DNuvec3){32, 14, 17}, (DNuvec3){55, 14, 13}, (DNuvec3){53, 14, 34}, (DNuvec3){28, 14, 45}, (DNuvec3){20, 14, 65}, (DNuvec3){55, 14, 60}};
	for(int i = 0; i < 8; i++)
	{
		DNuvec3 pos = spherePositions[i];

		for(int x = pos.x - 6; x <= pos.x + 6; x++)
			for(int y = pos.y - 6; y <= pos.y + 6; y++)
				for(int z = pos.z - 6; z <= pos.z + 6; z++)
				{
					DNvoxel vox;

					float distance = DN_vec3_distance((DNvec3){pos.x, pos.y, pos.z}, (DNvec3){x, y, z});
					if(distance >= 7.0f)
						continue;

					vox.material = i + 2;
					vox.normal = (DNvec3){x - (int)pos.x, y - (int)pos.y, z - (int)pos.z};

					float maxNormal = abs(vox.normal.x);
					if(abs(vox.normal.y) > maxNormal)
						maxNormal = abs(vox.normal.y);
					if(abs(vox.normal.z) > maxNormal)
						maxNormal = abs(vox.normal.z);

					vox.normal = DN_vec3_scale(vox.normal, 1 / maxNormal);

					DN_set_voxel(sphereMap, (DNivec3){x / 8, y / 8, z / 8}, (DNivec3){x % 8, y % 8, z % 8}, vox);
				}
	}*/

	//--------------//

	dnMaterials[0].albedo = DN_vec3_pow((DNvec3){0.8588f, 0.7922f, 0.6118f}, GAMMA);
	dnMaterials[0].emissive = false;
	dnMaterials[0].specular = 0;
	dnMaterials[0].opacity = 1.0f;

	dnMaterials[1].albedo = DN_vec3_pow((DNvec3){0.9f, 0.9f, 0.9f}, GAMMA);
	dnMaterials[1].emissive = false;
	dnMaterials[1].specular = 1.0f;
	dnMaterials[1].opacity = 1.0f;
	dnMaterials[1].reflectType = 1;
	dnMaterials[1].shininess = 100;

	dnMaterials[2].albedo = DN_vec3_pow((DNvec3){0.8784f, 0.3607f, 0.3607f}, GAMMA);
	dnMaterials[2].emissive = false;
	dnMaterials[2].specular = 0.0f;
	dnMaterials[2].opacity = 1.0f;

	dnMaterials[3].albedo = DN_vec3_pow((DNvec3){0.1f, 0.1f, 1.0f}, GAMMA);
	dnMaterials[3].emissive = false;
	dnMaterials[3].specular = 0.7f;
	dnMaterials[3].opacity = 1.0f;
	dnMaterials[3].reflectType = 0;
	dnMaterials[3].shininess = 3;

	dnMaterials[4].albedo = DN_vec3_pow((DNvec3){0.224f, 0.831f, 0.718f}, GAMMA);
	dnMaterials[4].emissive = true;
	dnMaterials[4].specular = 0.0f;
	dnMaterials[4].opacity = 1.0f;

	dnMaterials[5].albedo = DN_vec3_pow((DNvec3){1.0f, 1.0f, 1.0f}, GAMMA);
	dnMaterials[5].emissive = true;
	dnMaterials[5].specular = 0.0f;
	dnMaterials[5].opacity = 1.0f;

	dnMaterials[6].albedo = DN_vec3_pow((DNvec3){0.569f, 0.224f, 0.831f}, GAMMA);
	dnMaterials[6].emissive = false;
	dnMaterials[6].specular = 0.0f;
	dnMaterials[6].opacity = 0.5f;

	dnMaterials[7].albedo = DN_vec3_pow((DNvec3){0.8392f, 0.8314f, 0.2588f}, GAMMA);
	dnMaterials[7].emissive = false;
	dnMaterials[7].specular = 0.0f;
	dnMaterials[7].opacity = 0.5f;

	dnMaterials[8].albedo = DN_vec3_pow((DNvec3){0.8196f, 0.4549f, 0.1961f}, GAMMA);
	dnMaterials[8].emissive = true;
	dnMaterials[8].specular = 0.0f;
	dnMaterials[8].opacity = 1.0f;

	dnMaterials[9].albedo = DN_vec3_pow((DNvec3){0.306f, 0.831f, 0.224f}, GAMMA);
	dnMaterials[9].emissive = false;
	dnMaterials[9].specular = 0;
	dnMaterials[9].opacity = 1.0f;

	//sync with gpu:
	//---------------------------------
	DN_sync_gpu(treeMap, DN_READ_WRITE, DN_REQUEST_LOADED);
	DN_sync_gpu(sphereMap, DN_WRITE, DN_REQUEST_NONE);

	//set materials:
	//---------------------------------
	DN_set_materials(0, 10);

	//main loop:
	//---------------------------------
	float lastFrame = glfwGetTime();
	int numFrames = 0;
	float cumTime = 0.0;

	const int lightingSplit = 5;
	unsigned int numChunksToUpdate = 0;
	int frameNum = 0;
	float oldTime = 0.0f;

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
		DNmat3 rotate = DN_mat4_to_mat3(DN_mat4_rotate_euler(DN_MAT4_IDENTITY, (DNvec3){pitch, yaw, 0.0f}));
		camFront = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, fov });

		//render voxels to texture:
		activeMap->camPos = camPos;
		activeMap->camFOV = fov;
		activeMap->camOrient = (DNvec3){pitch, yaw, 0.0f};
		activeMap->camViewMode = viewMode;

		if(activeMap->streamable && updateData)
			DN_sync_gpu(activeMap, DN_READ_WRITE, DN_REQUEST_VISIBLE);
		DN_draw(activeMap);
		DN_update_lighting(activeMap, 5, 1, glfwGetTime());

		//render final quad to the screen:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, activeMap->glTextureID);
		DN_program_activate(quadShader);
		glBindVertexArray(quadBuffer);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		//finish rendering and swap:
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up and close:
	//---------------------------------
	DN_delete_map(treeMap);
	DN_quit();
	glDeleteVertexArrays(1, &quadBuffer);
	DN_program_free(quadShader);
	glfwTerminate();

	scanf("%d", &error);
	return 0;
}

void mouse_pos_callback(GLFWwindow *window, double x, double y)
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	DNvoxel hitVoxel;
	DNivec3 hitPos;
	DNivec3 hitNormal;

	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		if(DN_step_map(activeMap, DN_cam_dir(activeMap->camOrient), camPos, 64, &hitPos, &hitVoxel, &hitNormal))
		{
			DNivec3 newPos = {hitPos.x + hitNormal.x, hitPos.y + hitNormal.y, hitPos.z + hitNormal.z};

			DNivec3 mapPos;
			DNivec3 localPos;
			DN_separate_position(newPos, &mapPos, &localPos);

			if(DN_in_map_bounds(activeMap, mapPos))
			{
				DNvoxel newVox;
				newVox.material = 0;
				newVox.normal = (DNvec3){0.0f, 1.0f, 0.0f};

				DN_set_voxel(activeMap, mapPos, localPos, newVox);
				if(activeMap->streamable)
					DN_sync_gpu(activeMap, DN_WRITE, DN_REQUEST_NONE);
				else
					DN_sync_gpu(activeMap, DN_READ_WRITE, DN_REQUEST_LOADED);
			}
		}

	if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		if(DN_step_map(activeMap, DN_cam_dir(activeMap->camOrient), camPos, 64, &hitPos, &hitVoxel, &hitNormal))
		{
			DNivec3 mapPos;
			DNivec3 localPos;
			DN_separate_position(hitPos, &mapPos, &localPos);

			DN_remove_voxel(activeMap, mapPos, localPos);
			if(activeMap->streamable)
				DN_sync_gpu(activeMap, DN_WRITE, DN_REQUEST_NONE);
			else
				DN_sync_gpu(activeMap, DN_READ_WRITE, DN_REQUEST_LOADED);
		}
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

	if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
		updateData = true;
	if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
		updateData = false;
	if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
		activeMap = treeMap;
	if(glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS)
		activeMap = sphereMap;

	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camPos = DN_vec3_add(camPos, DN_vec3_scale(DN_vec3_normalize((DNvec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camPos = DN_vec3_subtract(camPos, DN_vec3_scale(DN_vec3_normalize((DNvec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camPos = DN_vec3_subtract(camPos, DN_vec3_scale(DN_vec3_normalize(DN_vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camPos = DN_vec3_add(camPos, DN_vec3_scale(DN_vec3_normalize(DN_vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camPos.y += camSpeed; 
	if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camPos.y -= camSpeed;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, w, h);
	DN_set_texture_size(treeMap, (DNuvec2){w, h});
	DN_set_texture_size(sphereMap, (DNuvec2){w, h});
	SCREEN_W = w;
	SCREEN_H = h;
}

void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if(severity == GL_DEBUG_SEVERITY_NOTIFICATION || type == 0x8250)
		return;

	DN_ERROR_LOG("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        	 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        	  type, severity, message );
}