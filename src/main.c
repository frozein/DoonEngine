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

//maps:
DNmap* activeMap;

DNmap* treeMap;
DNmap* demoMap;
DNmap* sphereMap;

//screen dimensions:
GLuint SCREEN_W = 1920;
GLuint SCREEN_H = 1080;
GLfloat ASPECT_RATIO = 9.0 / 16.0;

//cam stuff:
DNvec3 camFront =    {0.0f, 0.0f,  1.0f};
const DNvec3 camUp = {0.0f, 1.0f,  0.0f};

//timing:
float deltaTime = 0.0f;

//--------------------------------------------------------------------------------------------------------------------------------//

int main()
{
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
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	//load opengl functions:
	//---------------------------------
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD\n");
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
		return -1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER");
		glDeleteVertexArrays(1, &quadBuffer);
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
		return -1;
	}

	//load maps:
	//---------------------------------
	demoMap = DN_load_map("maps/demo.voxmap", (DNuvec2){SCREEN_W, SCREEN_H}, true, 128);
	demoMap->sunDir = (DNvec3){-1.0f, 1.0f, -1.0f};

	sphereMap = DN_load_map("maps/sphere.voxmap", (DNuvec2){SCREEN_W, SCREEN_H}, true, 1024);
	sphereMap->sunDir = (DNvec3){-1.0f, 1.0f, -1.0f};

	activeMap = demoMap;

	//load model:
	//---------------------------------
	treeMap = DN_create_map((DNuvec3){5, 5, 5}, (DNuvec2){SCREEN_W, SCREEN_H}, false, 0);
	treeMap->sunDir = (DNvec3){-1.0f, 1.0f, -1.0f};

	DNvoxelModel model;
	DN_load_vox_file("models/tree.vox", &model);
	DN_calculate_model_normals(2, &model);
	DN_place_model_into_map(treeMap, model, (DNivec3){0, 0, 0});

	treeMap->materials[130].albedo = DN_vec3_pow((DNvec3){0.4f, 0.2f, 0.0f}, DN_GAMMA);
	treeMap->materials[130].emissive = false;
	treeMap->materials[130].specular = 0.0f;
	treeMap->materials[130].opacity = 1.0f;

	treeMap->materials[160].albedo = DN_vec3_pow((DNvec3){0.2f, 0.4f, 0.0f}, DN_GAMMA);
	treeMap->materials[160].emissive = false;
	treeMap->materials[160].specular = 0.0f;
	treeMap->materials[160].opacity = 1.0f;

	//--------------//

	//sync with gpu:
	//---------------------------------
	DN_sync_gpu(treeMap, DN_READ_WRITE, DN_REQUEST_LOADED, 1);

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
		DNmat3 rotate = DN_mat4_to_mat3(DN_mat4_rotate_euler(DN_MAT4_IDENTITY, (DNvec3){activeMap->camOrient.x, activeMap->camOrient.y, 0.0f}));
		camFront = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, activeMap->camFOV });

		DN_draw(activeMap);
		if(activeMap->streamable)
			DN_sync_gpu(activeMap, DN_READ_WRITE, DN_REQUEST_VISIBLE, 5);
		DN_update_lighting(activeMap, 1, 1000, glfwGetTime());

		//render final quad to the screen:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, activeMap->glTextureID);
		DN_program_activate(quadShader);
		DN_program_uniform_float(quadShader, "time", glfwGetTime());
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
	DN_delete_map(demoMap);
	DN_delete_map(sphereMap);
	DN_quit();

	glDeleteVertexArrays(1, &quadBuffer);
	DN_program_free(quadShader);
	glfwTerminate();

	return 0;
}

void mouse_pos_callback(GLFWwindow *window, double x, double y)
{
	static float lastX = 400.0f, lastY = 300.0f;
	static bool firstMouse = true;

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

	activeMap->camOrient.y -= offsetX;
	activeMap->camOrient.x -= offsetY;

	if(activeMap->camOrient.x > 89.0f)
		activeMap->camOrient.x = 89.0f;
	if(activeMap->camOrient.x < -89.0f)
		activeMap->camOrient.x = -89.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	DNvoxel hitVoxel;
	DNivec3 hitPos;
	DNivec3 hitNormal;

	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		if(DN_step_map(activeMap, DN_cam_dir(activeMap->camOrient), activeMap->camPos, 64, &hitPos, &hitVoxel, &hitNormal))
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
				if(!activeMap->streamable)
					DN_sync_gpu(activeMap, DN_READ_WRITE, DN_REQUEST_LOADED, 1);
			}
		}

	if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		if(DN_step_map(activeMap, DN_cam_dir(activeMap->camOrient), activeMap->camPos, 64, &hitPos, &hitVoxel, &hitNormal))
		{
			DNivec3 mapPos;
			DNivec3 localPos;
			DN_separate_position(hitPos, &mapPos, &localPos);

			DN_remove_voxel(activeMap, mapPos, localPos);
			if(!activeMap->streamable)
				DN_sync_gpu(activeMap, DN_READ_WRITE, DN_REQUEST_LOADED, 1);
		}
}

void scroll_callback(GLFWwindow* window, double offsetX, double offsetY)
{
    activeMap->camFOV += (float)offsetY / 10.0f;
    if (activeMap->camFOV < 0.5f)
        activeMap->camFOV = 0.5f;
    if (activeMap->camFOV > 2.0f)
        activeMap->camFOV = 2.0f; 
}

void process_input(GLFWwindow *window)
{
	const float camSpeed = 3.0f * deltaTime;

	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		activeMap->camViewMode = 0;
	if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		activeMap->camViewMode = 1;
	if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		activeMap->camViewMode = 2;
	if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		activeMap->camViewMode = 3;
	if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		activeMap->camViewMode = 4;

	if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
		activeMap = demoMap;
	if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
		activeMap = treeMap;
	if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
		activeMap = sphereMap;

	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		activeMap->camPos = DN_vec3_add(activeMap->camPos, DN_vec3_scale(DN_vec3_normalize((DNvec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		activeMap->camPos = DN_vec3_subtract(activeMap->camPos, DN_vec3_scale(DN_vec3_normalize((DNvec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		activeMap->camPos = DN_vec3_subtract(activeMap->camPos, DN_vec3_scale(DN_vec3_normalize(DN_vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		activeMap->camPos = DN_vec3_add(activeMap->camPos, DN_vec3_scale(DN_vec3_normalize(DN_vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		activeMap->camPos.y += camSpeed; 
	if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		activeMap->camPos.y -= camSpeed;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, w, h);
	DN_set_texture_size(treeMap, (DNuvec2){w, h});
	DN_set_texture_size(demoMap, (DNuvec2){w, h});
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