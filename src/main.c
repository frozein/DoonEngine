#define GLFW_DLL

#include "QuickMath/quickmath.h"
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

#define STB_IMAGE_IMPLEMENTATION
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
//Prints any DN message to the console
void DN_message_callback(DNmessageType type, DNmessageSeverity severity, const char* message);

//--------------------------------------------------------------------------------------------------------------------------------//

//maps:
DNvolume* activeVol;

DNvolume* demoVol;
DNvolume* treeVol;
DNvolume* sphereVol;
DNvolume* cerealVol;

//rasterization textures:
GLuint rasterColorTex;
GLuint rasterDepthTex;
GLuint finalTex;

//rasterization FBO
GLuint rasterFBO;

//screen dimensions:
GLuint SCREEN_W = 1920;
GLuint SCREEN_H = 1080;

//cam stuff:
DNvec3 camFront =    {0.0f, 0.0f,  1.0f};
const DNvec3 camUp = {0.0f, 1.0f,  0.0f};

//timing:
float deltaTime = 0.0f;

//--------------------------------------------------------------------------------------------------------------------------------//

void place_cereal_bowl(DNvolume* vol, DNvoxel bowlVox, DNvec3 pos, float radius, int shape, unsigned int numColors, DNcolor* colors, unsigned int material)
{
	DN_shape_sphere(vol, bowlVox, false, pos, radius, NULL, NULL);
	bowlVox.material = DN_MATERIAL_EMPTY;
	DN_shape_sphere(vol, bowlVox, true, pos, radius - 5.0f, NULL, NULL);
	DN_shape_box(vol, bowlVox, true, (DNvec3){pos.x, pos.y + radius * 0.5f, pos.z}, (DNvec3){radius, radius * 0.5f, radius}, DN_quaternion_identity(), NULL, NULL);

	int numCereal = 0;
	switch(shape)
	{
	case 0:
		numCereal = 50;
		break;
	case 1:
		numCereal = 80;
		break;
	case 2:
		numCereal = 70;
		break;
	}

	for(int i = 0; i < numCereal; i++)
	{
		int a = radius * 2 + 1;
		int b = radius; 
		DNivec3 point = {rand() % a - b, -(rand() % b), rand() % a - b};

		if(point.x * point.x + point.y * point.y + point.z * point.z > (radius - 5.0f) * (radius - 5.0f))
		{
			i--;
			continue;
		}

		DNvoxel cerealVox;
		cerealVox.material = material;
		cerealVox.albedo = colors[rand() % numColors];
		DNvec3 finalPos = {pos.x + point.x, pos.y + point.y, pos.z + point.z};
		DNvec3 orientEuler = {rand() % 360, rand() % 360, rand() % 360};
		QMquaternion orient = DN_quaternion_from_euler(orientEuler);

		switch(shape)
		{
		case 0:
			DN_shape_torus(vol, cerealVox, false, finalPos, 5.0f, 3.0f, orient, NULL, NULL);
			break;
		case 1:
			DN_shape_sphere(vol, cerealVox, false, finalPos, 5.0f, NULL, NULL);
			break;
		case 2:
			DN_shape_box(vol, cerealVox, false, finalPos, (DNvec3){4.0f, 4.0f, 4.0f}, orient, NULL, NULL);
			break;
		}
	}
}

int main()
{
	srand(1234);

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
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(message_callback, 0);
	g_DN_message_callback = DN_message_callback;

	//set callback funcs:
	//---------------------------------
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback); 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//generate shader program:
	//---------------------------------
	int quadProgram = DN_program_load("shaders/quad.vert", NULL, "shaders/quad.frag", NULL);
	if(quadProgram < 0)
	{
		printf("Failed to load quad shader\n");
		DN_program_free(quadProgram);
		glfwTerminate();

		return -1;
	}

	DN_program_activate(quadProgram);
	DN_program_uniform_int(quadProgram, "colorTex", 0);

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
		printf("failed to generate final quad buffer\n");
		glDeleteVertexArrays(1, &quadBuffer);
		return -1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		printf("failed to generate final quad buffer");
		glDeleteVertexArrays(1, &quadBuffer);
		return -1;
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(long long)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(long long)(3 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(1);

	//load stuff for rasterization test:
	//---------------------------------

	//vertex data for a cube:
	float cubeVertices[] = {
		//positions:
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,

		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f
	};

	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	int cubeProgram = DN_program_load("shaders/vertTest.vert", NULL, "shaders/fragTest.frag", NULL);
	if(cubeProgram < 0)
	{
		printf("Failed to load rasterization test shader program\n");
		return -1;
	}

	//generate rasterization FBO:
	//---------------------------------
	glGenFramebuffers(1, &rasterFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, rasterFBO);

	//color texture:
	glGenTextures(1, &rasterColorTex);
	glBindTexture(GL_TEXTURE_2D, rasterColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rasterColorTex, 0);

	//depth texture:
	glGenTextures(1, &rasterDepthTex);
	glBindTexture(GL_TEXTURE_2D, rasterDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_W, SCREEN_H, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rasterDepthTex, 0);

	glEnable(GL_DEPTH_TEST);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("failed to create rasterization framebuffer\n");
		return -1;
	}

	//generate cubemap:
	//---------------------------------
	GLuint cubemapTex;
	glGenTextures(1, &cubemapTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	const char* paths[6] = {"textures/skybox/right.jpg", "textures/skybox/left.jpg", "textures/skybox/top.jpg", "textures/skybox/bottom.jpg", "textures/skybox/front.jpg", "textures/skybox/back.jpg"};
	for(int i = 0; i < 6; i++)
	{
		unsigned int w, h, nChannels;
		unsigned char* rawImage = stbi_load(paths[i], &w, &h, &nChannels, 0);
		if(!rawImage)
		{
			printf("failed to load skybox texture \"%s\"\n", paths[i]);
			continue;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rawImage);
		stbi_image_free(rawImage);
	}

	//generate final texture:
	//---------------------------------
	glGenTextures(1, &finalTex);
	glBindTexture(GL_TEXTURE_2D, finalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_W, SCREEN_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//initialize voxel pipeline:
	//---------------------------------
	if(!DN_init())
	{
		printf("Failed to initialize voxel pipeline\n");
		glfwTerminate();
		return -1;
	}

	//load volumes from disk:
	//---------------------------------
	demoVol   = DN_load_volume("volumes/demo.voxvol"  ,  128);
	sphereVol = DN_load_volume("volumes/sphere.voxvol",  2048);

	demoVol->glCubemapTex = cubemapTex;
	demoVol->useCubemap = true;

	//create volume with shapes:
	//---------------------------------
	cerealVol = DN_create_volume((DNuvec3){20, 20, 20}, 512);
	DNvoxel vox;
	cerealVol->sunDir = (DNvec3){-0.5f, 1.0f, -0.5f};
	cerealVol->glCubemapTex = cubemapTex;
	cerealVol->useCubemap = true;

	//place bowls:
	vox.albedo = (DNcolor){240, 240, 240};
	vox.material = 0;
	DNcolor rainbowColors[6] = {(DNcolor){242, 19, 19}, (DNcolor){242, 94, 19}, (DNcolor){242, 205, 19}, (DNcolor){34, 222, 13}, (DNcolor){39, 29, 224}, (DNcolor){113, 4, 201}};
	place_cereal_bowl(cerealVol, vox, (DNvec3){50.0f, 50.0f, 50.0f}, 40.0f, 0, 6, rainbowColors, 0);

	vox.albedo = (DNcolor){180, 180, 180};
	vox.material = 1;
	DNcolor chocolateColors[3] = {(DNcolor){64, 32, 13}, (DNcolor){87, 65, 51}, (DNcolor){66, 44, 23}};
	place_cereal_bowl(cerealVol, vox, (DNvec3){110.0f, 80.0f, 115.0f}, 40.0f, 2, 3, chocolateColors, 3);

	vox.albedo = (DNcolor){200, 200, 200};
	vox.material = 2;
	DNcolor trixColors[6] = {(DNcolor){4, 201, 192}, (DNcolor){88, 2, 168}, (DNcolor){214, 13, 26}, (DNcolor){227, 101, 5}, (DNcolor){100, 222, 24}, (DNcolor){212, 222, 24}};
	place_cereal_bowl(cerealVol, vox, (DNvec3){90.0f, 110.0f, 40.0f}, 40.0f, 1, 6, trixColors, 0);

	cerealVol->materials[0].emissive     = false;
	cerealVol->materials[0].specular     = 0.0f;
	cerealVol->materials[0].opacity      = 1.0f;

	cerealVol->materials[1].emissive     = false;
	cerealVol->materials[1].specular     = 0.8f;
	cerealVol->materials[1].opacity      = 1.0f;
	cerealVol->materials[1].shininess    = 3;
	cerealVol->materials[1].reflectType  = 1;

	cerealVol->materials[2].emissive     = false;
	cerealVol->materials[2].specular     = 0.0f;
	cerealVol->materials[2].opacity      = 0.5f;
	cerealVol->materials[2].refractIndex = 1.52f;

	cerealVol->materials[3].emissive     = true;
	cerealVol->materials[3].specular     = 0.0f;
	cerealVol->materials[3].opacity      = 1.0f;

	//load volume from MagicaVoxel model:
	//---------------------------------
	treeVol   = DN_create_volume((DNuvec3){5, 5, 5}, 64);

	DNvoxelModel model;
	DN_load_vox_file("models/tree.vox", 0, &model);
	DN_calculate_model_normals(2, &model);
	DN_place_model_into_volume(treeVol, model, (DNivec3){0, 0, 0});

	treeVol->sunDir = (DNvec3){-1.0f, 1.0f, -1.0f};
	treeVol->materials[0].emissive = false;
	treeVol->materials[0].specular = 0.0f;
	treeVol->materials[0].opacity = 1.0f;

	//main loop:
	//---------------------------------
	activeVol = demoVol;

	float lastFrame = glfwGetTime();
	int numFrames = 0;
	float cumTime = 0.0;

	while(!glfwWindowShouldClose(window))
	{
		//find deltatime:
		//---------------------------------
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
		//---------------------------------
		process_input(window);

		//update cam transform:
		//---------------------------------
		DNmat3 rotate = DN_mat4_top_left(DN_mat4_rotate_euler((DNvec3){activeVol->camOrient.x, activeVol->camOrient.y, 0.0f}));
		camFront = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, 1.0f });

		DNmat4 view;
		DNmat4 projection;
		DN_set_view_projection_matrices(activeVol, (float)SCREEN_H / SCREEN_W, 0.1f, 100.0f, &view, &projection);
		
		//rasterize objects:
		//---------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, rasterFBO);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		DN_program_activate(cubeProgram);

		DNmat4 model = DN_mat4_translate((DNvec3){5 + 3 * cosf(glfwGetTime()), 1.5 + cosf(glfwGetTime() * 5), 5 + 3 * sinf(glfwGetTime())});
		DN_program_uniform_mat4(cubeProgram, "modelMat", &model);
		DN_program_uniform_mat4(cubeProgram, "viewMat", &view);
		DN_program_uniform_mat4(cubeProgram, "projectionMat", &projection);
		DNvec3 color = {1.0f, 0.0f, 0.0f};
		DN_program_uniform_vec3(cubeProgram, "color", &color);

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//trace voxels:
		//---------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		DN_draw(activeVol, finalTex, view, projection, rasterColorTex, rasterDepthTex);
		DN_sync_gpu(activeVol, DN_READ_WRITE, 1);
		DN_update_lighting(activeVol, 1, 1000, glfwGetTime());

		//render final quad to the screen:
		//---------------------------------
		DN_program_activate(quadProgram);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, finalTex);
		
		glBindVertexArray(quadBuffer);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		//finish rendering and swap:
		//---------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up and close:
	//---------------------------------
	DN_delete_volume(treeVol);
	DN_delete_volume(demoVol);
	DN_delete_volume(sphereVol);
	DN_quit();

	glDeleteFramebuffers(1, &rasterFBO);
	glDeleteTextures(1, &rasterColorTex);
	glDeleteTextures(1, &rasterDepthTex);
	glDeleteTextures(1, &finalTex);

	glDeleteVertexArrays(1, &quadBuffer);
	DN_program_free(quadProgram);
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

	activeVol->camOrient.y -= offsetX;
	activeVol->camOrient.x -= offsetY;

	if(activeVol->camOrient.x > 89.0f)
		activeVol->camOrient.x = 89.0f;
	if(activeVol->camOrient.x < -89.0f)
		activeVol->camOrient.x = -89.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	DNvoxel hitVoxel;
	DNivec3 hitPos;
	DNivec3 hitNormal;

	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		if(DN_step_map(activeVol, DN_cam_dir(activeVol->camOrient), activeVol->camPos, 64, &hitPos, &hitVoxel, &hitNormal))
		{
			DNivec3 newPos = {hitPos.x + hitNormal.x, hitPos.y + hitNormal.y, hitPos.z + hitNormal.z};

			DNivec3 mapPos;
			DNivec3 localPos;
			DN_separate_position(newPos, &mapPos, &localPos);

			if(DN_in_map_bounds(activeVol, mapPos))
			{
				DNvoxel newVox;
				newVox.material = 0;
				newVox.normal = (DNvec3){0.0f, 1.0f, 0.0f};
				newVox.albedo = (DNcolor){250, 110, 121};

				DN_set_voxel(activeVol, mapPos, localPos, newVox);
			}
		}

	if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		if(DN_step_map(activeVol, DN_cam_dir(activeVol->camOrient), activeVol->camPos, 64, &hitPos, &hitVoxel, &hitNormal))
		{
			DNivec3 mapPos;
			DNivec3 localPos;
			DN_separate_position(hitPos, &mapPos, &localPos);

			DN_remove_voxel(activeVol, mapPos, localPos);
		}
}

void scroll_callback(GLFWwindow* window, double offsetX, double offsetY)
{
    activeVol->camFOV -= (float)offsetY;
    if (activeVol->camFOV < 45.0f)
        activeVol->camFOV = 45.0f;
    if (activeVol->camFOV > 90.0f)
        activeVol->camFOV = 90.0f; 
}

void process_input(GLFWwindow *window)
{
	const float camSpeed = 3.0f * deltaTime;

	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		activeVol->camViewMode = 0;
	if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		activeVol->camViewMode = 1;
	if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		activeVol->camViewMode = 2;
	if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		activeVol->camViewMode = 3;
	if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		activeVol->camViewMode = 4;
	if(glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		activeVol->camViewMode = 5;

	if(glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
		activeVol = demoVol;
	if(glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
		activeVol = treeVol;
	if(glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
		activeVol = sphereVol;
	if(glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS)
		activeVol = cerealVol;

	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		activeVol->camPos = DN_vec3_add(activeVol->camPos, DN_vec3_scale(DN_vec3_normalize((DNvec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		activeVol->camPos = DN_vec3_sub(activeVol->camPos, DN_vec3_scale(DN_vec3_normalize((DNvec3){camFront.x, 0.0f, camFront.z}), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		activeVol->camPos = DN_vec3_sub(activeVol->camPos, DN_vec3_scale(DN_vec3_normalize(DN_vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		activeVol->camPos = DN_vec3_add(activeVol->camPos, DN_vec3_scale(DN_vec3_normalize(DN_vec3_cross(camFront, camUp)), camSpeed));
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		activeVol->camPos.y += camSpeed; 
	if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		activeVol->camPos.y -= camSpeed;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	glViewport(0, 0, w, h);

	SCREEN_W = w;
	SCREEN_H = h;

	glBindFramebuffer(GL_FRAMEBUFFER, rasterFBO);
	glBindTexture(GL_TEXTURE_2D, rasterColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rasterColorTex, 0);
	glBindTexture(GL_TEXTURE_2D, rasterDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_W, SCREEN_H, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rasterDepthTex, 0);
	glBindTexture(GL_TEXTURE_2D, finalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCREEN_W, SCREEN_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if(severity == GL_DEBUG_SEVERITY_NOTIFICATION || type == 0x8250)
		return;

	printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        	 (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        	  type, severity, message );
}

void DN_message_callback(DNmessageType type, DNmessageSeverity severity, const char* message)
{
	printf("DN MESSAGE: type = %d, severity = %d, message = %s\n", type, severity, message);
}