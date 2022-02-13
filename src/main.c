#define GLFW_DLL

#include "DoonEngine/math/all.h"
#include "DoonEngine/shader.h"
#include "DoonEngine/render.h"
#include "DoonEngine/texture.h"
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
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

//--------------------------------------------------------------------------------------------------------------------------------//

//for the full screen quad that gets rendered at the end:
GLfloat quadVertices[] = {
	 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
};
GLuint quadIndices[] = {
	0, 1, 3,
	1, 2, 3
};

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

unsigned int viewMode = 0;

//--------------------------------------------------------------------------------------------------------------------------------//

//voxel map stuff:
#define CHUNK_SIZE_X 8
#define CHUNK_SIZE_Y 8
#define CHUNK_SIZE_Z 8

#define MAP_SIZE_X 3
#define MAP_SIZE_Y 3
#define MAP_SIZE_Z 3

#define MAX_CHUNKS 11
#define MAX_LIGHTING_REQUESTS 11

typedef struct Voxel
{
	vec3 color;
	GLint material;

	vec3 accumColor;
	GLfloat numSamples;
} Voxel;

typedef struct Chunk
{
	Voxel voxels[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
} Chunk;

typedef struct ivec4
{
	GLint x, y, z, w;
} ivec4;

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

	//set callback func:
	//---------------------------------
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback); 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//generate shader program:
	//---------------------------------
	int quadShader = shader_program_load("shaders/quad.vert", NULL, "shaders/quad.frag", NULL);
	int voxelShader = compute_shader_program_load("shaders/voxel.comp", "shaders/voxelShared.comp");
	int voxelLightingShader = compute_shader_program_load("shaders/voxelLighting.comp", "shaders/voxelShared.comp");
	if(quadShader < 0 || voxelShader < 0 || voxelLightingShader < 0)
	{
		shader_program_free(voxelLightingShader >= 0 ? voxelLightingShader : 0);
		shader_program_free(voxelShader >= 0 ? voxelShader : 0);
		shader_program_free(quadShader >= 0 ? quadShader : 0);

		glfwTerminate();
		return -1;
	}

	//generate quad buffer:
	//---------------------------------
	unsigned int quadBuffer = gen_vertex_object_indices(sizeof(quadVertices), quadVertices, sizeof(quadIndices), quadIndices, DRAW_STATIC);
	set_vertex_attribute(quadBuffer, 0, 3, GL_FLOAT, 5 * sizeof(GLfloat), 0);
	set_vertex_attribute(quadBuffer, 1, 2, GL_FLOAT, 5 * sizeof(GLfloat), 3 * sizeof(GLfloat));

	//create and bind uniform buffer object
	//---------------------------------
	shader_program_activate(voxelShader);

	unsigned int mapBuffer;
	glGenBuffers(1, &mapBuffer); //generate buffer object
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * MAP_SIZE_X * MAP_SIZE_Y * MAP_SIZE_Z, NULL, GL_DYNAMIC_DRAW); //allocate
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mapBuffer); //bind to 0th index buffer binding, also defined in the shader

	unsigned int chunkBuffer;
	glGenBuffers(1, &chunkBuffer); //generate buffer object
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Chunk) * MAX_CHUNKS, NULL, GL_DYNAMIC_DRAW); //allocate
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkBuffer); //bind to 1st index buffer binding, also defined in the shader

	unsigned int lightingRequestBuffer;
	glGenBuffers(1, &lightingRequestBuffer); //generate buffer object
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * MAX_LIGHTING_REQUESTS, NULL, GL_DYNAMIC_DRAW); //allocate
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lightingRequestBuffer); //bind to 1st index buffer binding, also defined in the shader

	//generate voxel data:
	//---------------------------------
	Chunk* chunks = malloc(sizeof(Chunk) * MAX_CHUNKS);
	for(int x = 0; x < CHUNK_SIZE_X; x++) //create colored sphere
		for(int y = 0; y < CHUNK_SIZE_Y; y++)
			for(int z = 0; z < CHUNK_SIZE_Z; z++)
			{
				//cylinder:
				for(int i = 0; i < 2; i++)
				{
					float distance = vec2_distance((vec2){4, 4}, (vec2){x, z});
					chunks[i].voxels[x][y][z].material = (distance < 4.0f) - 1;
					chunks[i].voxels[x][y][z].color.x = x / 8.0f;
					chunks[i].voxels[x][y][z].color.y = y / 8.0f;
					chunks[i].voxels[x][y][z].color.z = z / 8.0f;
					chunks[i].voxels[x][y][z].accumColor = (vec3){0.0f, 0.0f, 0.0f};
					chunks[i].voxels[x][y][z].numSamples = 0.0f;
				}

				//block:
				for(int i = 2; i < 11; i++)
				{
					chunks[i].voxels[x][y][z].material = 0;
					chunks[i].voxels[x][y][z].color = (vec3){0.8588f, 0.7922f, 0.6118f};
					chunks[i].voxels[x][y][z].accumColor = (vec3){0.0f, 0.0f, 0.0f};
					chunks[i].voxels[x][y][z].numSamples = 0.0f;
				}
			}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Chunk) * MAX_CHUNKS, chunks);

	//--------------//

	vec4* map = malloc(sizeof(vec4) * MAP_SIZE_X * MAP_SIZE_Y * MAP_SIZE_Z); //needs to be a vec4 for alignment
	for(int x = 0; x < MAP_SIZE_X; x++)
		for(int y = 0; y < MAP_SIZE_Y; y++)
			for(int z = 0; z < MAP_SIZE_Z; z++)
			{
				if(y == 0)
					(GLint)map[x + MAP_SIZE_X * y + MAP_SIZE_X * MAP_SIZE_Y * z].x = 2 + x + z * 3;
				else if(x == 1 && z == 1)
					(GLint)map[x + MAP_SIZE_X * y + MAP_SIZE_X * MAP_SIZE_Y * z].x = y - 1;
				else
					(GLint)map[x + MAP_SIZE_X * y + MAP_SIZE_X * MAP_SIZE_Y * z].x = -1;
			}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * MAP_SIZE_X * MAP_SIZE_Y * MAP_SIZE_Z, map);

	//--------------//

	ivec4* lightingRequests = malloc(sizeof(ivec4) * MAX_CHUNKS);
	lightingRequests[0]  = (ivec4){0, 0, 0};
	lightingRequests[1]  = (ivec4){0, 0, 1};
	lightingRequests[2]  = (ivec4){0, 0, 2};
	lightingRequests[3]  = (ivec4){1, 0, 0};
	lightingRequests[4]  = (ivec4){1, 0, 1};
	lightingRequests[5]  = (ivec4){1, 0, 2};
	lightingRequests[6]  = (ivec4){2, 0, 0};
	lightingRequests[7]  = (ivec4){2, 0, 1};
	lightingRequests[8]  = (ivec4){2, 0, 2};
	lightingRequests[9]  = (ivec4){1, 1, 1};
	lightingRequests[10] = (ivec4){1, 2, 1};

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * MAX_LIGHTING_REQUESTS, lightingRequests);

	//generate texture:
	//---------------------------------
	Texture tex = texture_load_raw(GL_TEXTURE_2D, SCREEN_W, SCREEN_H, GL_RGBA, NULL, false);
	texture_param_scale(tex, GL_LINEAR, GL_LINEAR);
	texture_param_wrap(tex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	glBindImageTexture(0, tex.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

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

		//update lighting:
		shader_program_activate(voxelLightingShader);

		shader_uniform_float(voxelLightingShader, "time", glfwGetTime());

		glDispatchCompute(MAX_LIGHTING_REQUESTS, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		//draw:
		shader_program_activate(voxelShader);

		shader_uniform_vec3(voxelShader, "camPos", camPos);
		shader_uniform_vec3(voxelShader, "camDir", camFront);
		shader_uniform_vec3(voxelShader, "camPlaneU", camPlaneU);
		shader_uniform_vec3(voxelShader, "camPlaneV", camPlaneV);
		shader_uniform_uint(voxelShader, "viewMode", viewMode);

		glDispatchCompute(SCREEN_W / 16, SCREEN_H / 16, 1); //TODO: ALLOW FOR WINDOW RESIZING, NEED TO REGEN TEXTURE EACH TIME
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		//draw final quad to screen:
		shader_program_activate(quadShader);
		texture_activate(tex, 0);
		draw_vertex_object_indices(quadBuffer, 6, 0);

		//finish rendering and swap:
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up and close:
	//---------------------------------
	texture_free(tex);

	glDeleteBuffers(1, &chunkBuffer);
	glDeleteBuffers(1, &mapBuffer);
	glDeleteBuffers(1, &lightingRequestBuffer);
	free_vertex_object(quadBuffer);

	shader_program_free(voxelLightingShader);
	shader_program_free(voxelShader);
	shader_program_free(quadShader);

	free(chunks);
	free(map);
	free(lightingRequests);

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