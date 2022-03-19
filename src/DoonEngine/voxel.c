#include "voxel.h"
#include "shader.h"
#include "texture.h"
#include "globals.h"
#include <malloc.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//shader handles:
unsigned int directLightingShader = 0;
unsigned int indirectLightingShader = 0;
unsigned int finalShader = 0;

//buffer handles:
unsigned int mapBuffer = 0;
unsigned int chunkBuffer = 0;
unsigned int materialBuffer = 0;
unsigned int lightingRequestBuffer = 0;

Texture finalTex = {0, GL_TEXTURE_2D};

//memory meta-data:
uvec2 textureSize = {0, 0};

uvec3 mapSize = {0, 0, 0};
unsigned int maxChunks = 0;

uvec3 mapSizeGPU = {0, 0, 0};
unsigned int maxChunksGPU = 0;
unsigned int maxLightingRequests = 0;

//cpu-side memory:
int* voxelMap = 0;
VoxelChunk* voxelChunks = 0;
VoxelMaterial* voxelMaterials = 0;
ivec4* voxelLightingRequests = 0;

//lighting parameters:
vec3 sunDir = {-1.0f, 1.0f, -1.0f};
vec3 sunStrength = {0.6f, 0.6f, 0.6f};
float ambientStrength = 0.01f;
unsigned int bounceLimit = 5;
float bounceStrength = 1.0f;
float shadowSoftness = 10.0f;
unsigned int viewMode = 0;

//--------------------------------------------------------------------------------------------------------------------------------//

static bool gen_shader_storage_buffer(unsigned int* dest, size_t size, unsigned int binding);

bool init_voxel_pipeline(uvec2 tSize, Texture fTex, uvec3 mSize, unsigned int mChunks, uvec3 mSizeGPU, unsigned int mChunksGPU, unsigned int mLightingRequests)
{
	//assign variables:
	//---------------------------------
	textureSize = tSize;
	finalTex = fTex;
	mapSize = mSize;
	maxChunks = mChunks;
	mapSizeGPU = mSizeGPU;
	maxChunksGPU = mChunksGPU;
	maxLightingRequests = mLightingRequests;

	//bind texture:
	//---------------------------------
	glBindImageTexture(0, finalTex.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//allocate cpu-side memory:
	//---------------------------------
	voxelMap = malloc(sizeof(int) * mapSize.x * mapSize.y * mapSize.z);
	if(!voxelMap)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL MAP\n");
		texture_free(finalTex);
		return false;
	}

	voxelChunks = malloc(sizeof(VoxelChunk) * maxChunks);
	if(!voxelChunks)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL CHUNKS\n");
		texture_free(finalTex);
		free(voxelMap);
		return false;
	}

	voxelMaterials = malloc(sizeof(VoxelMaterial) * MAX_MATERIALS);
	if(!voxelMaterials)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL MATERIALS\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		return false;
	}

	voxelLightingRequests = malloc(sizeof(ivec4) * maxLightingRequests);
	if(!voxelLightingRequests)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL LIGHTING REQUESTS\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		free(voxelMaterials);
		return false;
	}

	//generate buffers:
	//---------------------------------
	if(!gen_shader_storage_buffer(&mapBuffer, sizeof(ivec4) * mapSizeGPU.x * mapSizeGPU.y * mapSizeGPU.x, 0))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL MAP BUFFER\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		free(voxelMaterials);
		free(voxelLightingRequests);
		return false;
	}

	if(!gen_shader_storage_buffer(&chunkBuffer, sizeof(VoxelChunk) * 2 * maxChunksGPU, 1))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL CHUNK BUFFER\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		free(voxelMaterials);
		free(voxelLightingRequests);
		glDeleteBuffers(1, &mapBuffer);
		return false;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

	if(!gen_shader_storage_buffer(&materialBuffer, sizeof(VoxelMaterial) * MAX_MATERIALS, 2))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL MATERIAL BUFFER\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		free(voxelMaterials);
		free(voxelLightingRequests);
		glDeleteBuffers(1, &mapBuffer);
		glDeleteBuffers(1, &chunkBuffer);
		return false;
	}

	if(!gen_shader_storage_buffer(&lightingRequestBuffer, sizeof(ivec4) * maxLightingRequests, 3))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL LIGHTING REQUEST BUFFER\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		free(voxelMaterials);
		free(voxelLightingRequests);
		glDeleteBuffers(1, &mapBuffer);
		glDeleteBuffers(1, &chunkBuffer);
		glDeleteBuffers(1, &materialBuffer);
		return false;
	}

	//load shaders:
	//---------------------------------
	int direct   = compute_shader_program_load("shaders/voxelDirectLight.comp", "shaders/voxelShared.comp");
	int indirect = compute_shader_program_load("shaders/voxelGlobalLight.comp", "shaders/voxelShared.comp");
	int final    = compute_shader_program_load("shaders/voxelFinal.comp"      , "shaders/voxelShared.comp");

	if(direct < 0 || indirect < 0 || final < 0)
	{
		ERROR_LOG("ERROR - FAILED TO COMPILE 1 OR MORE VOXEL SHADERS\n");
		texture_free(finalTex);
		free(voxelMap);
		free(voxelChunks);
		free(voxelMaterials);
		free(voxelLightingRequests);
		glDeleteBuffers(1, &mapBuffer);
		glDeleteBuffers(1, &chunkBuffer);
		glDeleteBuffers(1, &materialBuffer);
		glDeleteBuffers(1, &lightingRequestBuffer);
		shader_program_free(direct   >= 0 ? direct : 0);
		shader_program_free(indirect >= 0 ? indirect : 0);
		shader_program_free(final    >= 0 ? final : 0);

		return false;
	}

	directLightingShader = direct;
	indirectLightingShader = indirect;
	finalShader = final;

	//return:
	//---------------------------------
	return true;
}

void deinit_voxel_pipeline()
{
	shader_program_free(directLightingShader);
	shader_program_free(indirectLightingShader);
	shader_program_free(finalShader);

	glDeleteBuffers(1, &mapBuffer);
	glDeleteBuffers(1, &chunkBuffer);
	glDeleteBuffers(1, &materialBuffer);
	glDeleteBuffers(1, &lightingRequestBuffer);

	free(voxelMap);
	free(voxelChunks);
	free(voxelMaterials);
	free(voxelLightingRequests);

	texture_free(finalTex);
}

void update_voxel_indirect_lighting(unsigned int numChunks, float time)
{
	shader_program_activate(indirectLightingShader);

	shader_uniform_vec3(indirectLightingShader, "sunDir", vec3_normalize(sunDir));
	shader_uniform_vec3(indirectLightingShader, "sunStrength", sunStrength);

	shader_uniform_int(indirectLightingShader, "bounceLimit", bounceLimit);
	shader_uniform_float(indirectLightingShader, "bounceStrength", bounceStrength);

	shader_uniform_float(indirectLightingShader, "time", time);

	glDispatchCompute(numChunks, 1, 1);
}

void update_voxel_direct_lighting(unsigned int numChunks, vec3 camPos)
{
	shader_program_activate(directLightingShader);

	shader_uniform_vec3 (directLightingShader, "sunDir", vec3_normalize(sunDir));
	shader_uniform_vec3(directLightingShader, "sunStrength", sunStrength);
	shader_uniform_float(directLightingShader, "shadowSoftness", shadowSoftness);
	shader_uniform_float(directLightingShader, "ambientStrength", ambientStrength);
	shader_uniform_vec3 (directLightingShader, "camPos", camPos);

	glDispatchCompute(maxLightingRequests, 1, 1);
}

void draw_voxels(vec3 camPos, vec3 camFront, vec3 camPlaneU, vec3 camPlaneV)
{
	shader_program_activate(finalShader);

	shader_uniform_vec3 (finalShader, "camPos", camPos);
	shader_uniform_vec3 (finalShader, "camDir", camFront);
	shader_uniform_vec3 (finalShader, "camPlaneU", camPlaneU);
	shader_uniform_vec3 (finalShader, "camPlaneV", camPlaneV);
	shader_uniform_uint (finalShader, "viewMode", viewMode);
	shader_uniform_float(finalShader, "ambientStrength", ambientStrength);
	shader_uniform_vec3(finalShader, "sunStrength", sunStrength);

	glDispatchCompute(textureSize.x / 16, textureSize.y / 16, 1);
}

void send_all_data_temp()
{
	ivec4* tempMap = malloc(sizeof(ivec4) * mapSizeGPU.x * mapSizeGPU.y * mapSizeGPU.z);

	for(int x = 0; x < mapSize.x; x++)
		for(int y = 0; y < mapSize.y; y++)
			for(int z = 0; z < mapSize.z; z++)
			{
				tempMap[x + mapSize.x * y + mapSize.x * mapSize.y * z].x = voxelMap[x + mapSize.x * y + mapSize.x * mapSize.y * z];
			}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);
	for(int i = 0; i < maxChunksGPU; i++)
	{
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * 2 * sizeof(VoxelChunk), sizeof(VoxelChunk), &voxelChunks[i]);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ivec4) * mapSizeGPU.x * mapSizeGPU.y * mapSizeGPU.z, tempMap);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(VoxelMaterial) * MAX_MATERIALS, voxelMaterials);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * maxLightingRequests, voxelLightingRequests);
}

//--------------------------------------------------------------------------------------------------------------------------------//

uvec2 texture_size()
{
	return textureSize;
}

bool set_texture_size(uvec2 size)
{
	return true; //TODO implement
}

uvec3 map_size()
{
	return mapSize;
}

bool set_map_size(uvec3 size)
{
	return true; //TODO implement
}

unsigned int max_chunks()
{
	return maxChunks;
}

unsigned int current_chunks()
{
	return 0; //TODO implement
}

bool set_max_chunks(unsigned int num)
{
	return true; //TODO implement
}

//--------------------------------------------------------------------------------------------------------------------------------//

uvec3 gpu_map_size()
{
	return mapSizeGPU;
}

bool set_gpu_map_size(uvec3 size)
{
	return true; //TODO implement
}

unsigned int max_gpu_chunks()
{
	return maxChunksGPU;
}

bool set_max_gpu_chunks(unsigned int num)
{
	return true; //TODO implement
}

unsigned int max_lighting_requests()
{
	return maxLightingRequests;
}

bool set_max_lighting_requests(unsigned int num)
{
	return true; //TODO implement
}

//--------------------------------------------------------------------------------------------------------------------------------//

static GLuint encode_uint_RGBA(uvec4 val);
static uvec4 decode_uint_RGBA(GLuint val);

VoxelGPU voxel_to_voxelGPU(Voxel voxel)
{
	VoxelGPU res;
	uvec4 albedo = {(GLuint)(voxel.albedo.x * 255), (GLuint)(voxel.albedo.y * 255), (GLuint)(voxel.albedo.z * 255), voxel.material};
	uvec4 normal = {(GLuint)((voxel.normal.x * 0.5 + 0.5) * 255), (GLuint)((voxel.normal.y * 0.5 + 0.5) * 255), (GLuint)((voxel.normal.z * 0.5 + 0.5) * 255), 0};
	uvec4 nullVec = {0, 0, 0, 0};

	res.albedo = encode_uint_RGBA(albedo);
	res.normal = encode_uint_RGBA(normal);
	res.directLight = encode_uint_RGBA(nullVec);
	res.specLight = encode_uint_RGBA(nullVec);

	return res;
}

Voxel voxelGPU_to_voxel(VoxelGPU voxel)
{
	Voxel res;
	uvec4 albedo = decode_uint_RGBA(voxel.albedo);
	uvec4 normal = decode_uint_RGBA(voxel.normal);

	res.albedo = vec3_scale((vec3){albedo.x, albedo.y, albedo.z}, 0.00392156862);
	vec3 scaledNormal = vec3_scale((vec3){normal.x, normal.y, normal.z}, 0.00392156862);
	res.normal = (vec3){(scaledNormal.x - 0.5) * 2.0, (scaledNormal.y - 0.5) * 2.0, (scaledNormal.z - 0.5) * 2.0};
	res.material = albedo.w;

	return res;
}

//--------------------------------------------------------------------------------------------------------------------------------//

bool in_map_bounds(ivec3 pos) //returns whether a position is in bounds in the map
{
	return pos.x < mapSize.x && pos.y < mapSize.y && pos.z < mapSize.z && pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

bool in_chunk_bounds(ivec3 pos) //returns whether a position is in bounds in a chunk
{
	return pos.x < CHUNK_SIZE_X && pos.y < CHUNK_SIZE_Y && pos.z < CHUNK_SIZE_Z && pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

int get_map_tile(ivec3 pos) //returns the value of the map at a position DOESNT DO ANY BOUNDS CHECKING
{
	return voxelMap[pos.x + mapSize.x * (pos.y + mapSize.y * pos.z)];
}

VoxelGPU get_voxel(unsigned int chunk, ivec3 pos) //returns the voxel of a chunk at a position DOESNT DO ANY BOUNDS CHECKING
{
	return voxelChunks[chunk].voxels[pos.x][pos.y][pos.z];
}

//--------------------------------------------------------------------------------------------------------------------------------//

static bool gen_shader_storage_buffer(unsigned int* dest, size_t size, unsigned int binding)
{
	unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		glDeleteBuffers(1, &buffer);
		return false;
	}

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer); //bind to 0th index buffer binding, also defined in the shader

	*dest = buffer;

	return true;
}

static GLuint encode_uint_RGBA(uvec4 val)
{
	val.x = val.x & 0xFF;
	val.y = val.y & 0xFF;
	val.z = val.z & 0xFF;
	val.w = val.w & 0xFF;
	return val.x << 24 | val.y << 16 | val.z << 8 | val.w;
}

static uvec4 decode_uint_RGBA(GLuint val)
{
	uvec4 res;
	res.x = (val >> 24) & 0xFF;
	res.y = (val >> 16) & 0xFF;
	res.z = (val >> 8) & 0xFF;
	res.w = (val) & 0xFF;
	return res;
}