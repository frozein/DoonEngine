#include "voxel.h"
#include "shader.h"
#include "texture.h"
#include "globals.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>

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
VoxelChunkHandle* voxelMap = 0;
VoxelChunk* voxelChunks = 0;
VoxelMaterial* voxelMaterials = 0;
ivec4* voxelLightingRequests = 0;

ivec4* chunkBufferLayout; //how the chunk buffer is laid out on the gpu, used for chunk streaming

//lighting parameters:
vec3 sunDir = {-1.0f, 1.0f, -1.0f};
vec3 sunStrength = {0.6f, 0.6f, 0.6f};
float ambientStrength = 0.01f;
unsigned int bounceLimit = 5;
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
	voxelMap = malloc(sizeof(VoxelChunkHandle) * mapSize.x * mapSize.y * mapSize.z);
	if(!voxelMap)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL MAP\n");
		return false;
	}

	for(int i = 0; i < mapSize.x * mapSize.y * mapSize.z; i++)
		voxelMap[i].flag = voxelMap[i].visible = 0;

	voxelChunks = malloc(sizeof(VoxelChunk) * maxChunks);
	if(!voxelChunks)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL CHUNKS\n");
		return false;
	}

	for(int i = 0; i < maxChunks; i++)
		for(int z = 0; z < CHUNK_SIZE_Z; z++)
			for(int y = 0; y < CHUNK_SIZE_Y; y++)
				for(int x = 0; x < CHUNK_SIZE_X; x++)
					voxelChunks[i].voxels[x][y][z].albedo = UINT32_MAX;

	chunkBufferLayout = malloc(sizeof(ivec4) * maxChunksGPU);
	if(!chunkBufferLayout)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR CHUNK BUFFER LAYOUT\n");
		return false;
	}

	for(int i = 0; i < maxChunksGPU; i++)
		chunkBufferLayout[i] = (ivec4){-1, -1, -1, -1};

	voxelMaterials = malloc(sizeof(VoxelMaterial) * MAX_MATERIALS);
	if(!voxelMaterials)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL MATERIALS\n");
		return false;
	}

	voxelLightingRequests = malloc(sizeof(ivec4) * maxLightingRequests);
	if(!voxelLightingRequests)
	{
		ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL LIGHTING REQUESTS\n");
		return false;
	}

	//generate buffers:
	//---------------------------------
	if(!gen_shader_storage_buffer(&mapBuffer, sizeof(ivec4) * mapSizeGPU.x * mapSizeGPU.y * mapSizeGPU.x, 0))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL MAP BUFFER\n");
		return false;
	}

	if(!gen_shader_storage_buffer(&chunkBuffer, (sizeof(VoxelChunk) + sizeof(vec4) * CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z) * maxChunksGPU, 1))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL CHUNK BUFFER\n");
		return false;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

	if(!gen_shader_storage_buffer(&materialBuffer, sizeof(VoxelMaterial) * MAX_MATERIALS, 2))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL MATERIAL BUFFER\n");
		return false;
	}

	if(!gen_shader_storage_buffer(&lightingRequestBuffer, sizeof(ivec4) * maxLightingRequests, 3))
	{
		ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL LIGHTING REQUEST BUFFER\n");
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
	free(chunkBufferLayout);
	free(voxelMaterials);
	free(voxelLightingRequests);

	texture_free(finalTex);
}

//--------------------------------------------------------------------------------------------------------------------------------//

void update_gpu_voxel_data()
{
	const int maxGpuMapIndex = mapSizeGPU.x * mapSizeGPU.y * mapSizeGPU.z; //the maximum map index on the GPU
	const size_t gpuChunkSize = sizeof(VoxelChunk) + sizeof(vec4) * CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z; //the size of a voxel chunk on the GPU

	//map the buffer:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	uvec4* voxelMapGPU = (uvec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	//bind the chunk buffer so it can be updated:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);

	//loop through every map tile:
	for(int z = 0; z < mapSizeGPU.z; z++)
	for(int y = 0; y < mapSizeGPU.y; y++)
	for(int x = 0; x < mapSizeGPU.x; x++)
	{
		int cpuMapIndex = FLATTEN_INDEX(x, y, z, mapSize);    //the map index for the cpu-side memory
		int gpuMapIndex = FLATTEN_INDEX(x, y, z, mapSizeGPU); //the map index for the gpu-side memory

		voxelMapGPU[gpuMapIndex].y = 0; //set the "visible" flag to 0

		if(voxelMapGPU[gpuMapIndex].z < UINT32_MAX) //increase the "time last used" flag
			voxelMapGPU[gpuMapIndex].z++;

		if(voxelMapGPU[gpuMapIndex].x == 3) //if flag = 3 (requested), try to load a new chunk
		{
			//variables that belong to the oldest chunk:
			int maxTime = 0;
			int maxTimeIndex = -1;
			int maxTimeMapIndex = -1;

			//loop through every chunk to look for oldest:
			for(int i = 0; i < maxChunksGPU; i++)
			{
				ivec4 chunkPos = chunkBufferLayout[i];
				int chunkMapIndex = FLATTEN_INDEX(chunkPos.x, chunkPos.y, chunkPos.z, mapSizeGPU);

				//if chunk does not belong to any active map tile, instantly use it
				if(chunkPos.w < 0 || chunkMapIndex >= maxGpuMapIndex)
				{
					maxTime = 2;
					maxTimeIndex = i;
					maxTimeMapIndex = -1;
					break;
				}
				else if(voxelMapGPU[chunkMapIndex].z > maxTime) //else, check if it is older than the current oldest
				{
					maxTime = voxelMapGPU[chunkMapIndex].z;
					maxTimeIndex = i;
					maxTimeMapIndex = chunkMapIndex;
				}
			}

			if(maxTime > 1) //only load in the new chunk if the old one isnt currently in use
			{
				if(maxTimeMapIndex >= 0) //if the chunk was previously loaded, set its flag to unloaded
					voxelMapGPU[maxTimeMapIndex].x = voxelMapGPU[maxTimeMapIndex].x > 0 ? 2 : 0;

				voxelMapGPU[gpuMapIndex].x = 1; //set the new tile's flag to loaded
				voxelMapGPU[gpuMapIndex].w = maxTimeIndex;

				chunkBufferLayout[maxTimeIndex] = (ivec4){x, y, z, 0}; //update the chunk layout

				glBufferSubData(GL_SHADER_STORAGE_BUFFER, maxTimeIndex * gpuChunkSize, sizeof(VoxelChunk), &voxelChunks[cpuMapIndex]); //send the data
			}
			else //if the oldest chunk is currently in use, double the buffer size
			{
				ERROR_LOG("NOTE - MAXIMUM GPU CHUNK LIMIT REACHED... RESIZING TO %i CHUNKS\n", maxChunksGPU * 2);

				//create a temporary buffer to store the old chunk data (opengl doesnt have a "realloc" function)
				void* oldChunkData = malloc(gpuChunkSize * maxChunksGPU);
				if(!oldChunkData)
				{
					ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR TEMPORARY CHUNK STORAGE\n");
					return;
				}

				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuChunkSize * maxChunksGPU, oldChunkData); //get old data
				set_max_voxel_chunks_gpu(maxChunksGPU * 2); //resize buffer
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuChunkSize * (maxChunksGPU / 2), oldChunkData); //send back old data

				free(oldChunkData); //free temporary memory
			}
		}
	}

	//unmap:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void update_voxel_indirect_lighting(unsigned int numChunks, float time)
{
	shader_program_activate(indirectLightingShader);

	shader_uniform_vec3(indirectLightingShader, "sunDir", vec3_normalize(sunDir));
	shader_uniform_vec3(indirectLightingShader, "sunStrength", sunStrength);
	shader_uniform_int(indirectLightingShader, "bounceLimit", bounceLimit);
	shader_uniform_float(indirectLightingShader, "time", time);
	glUniform3uiv(glGetUniformLocation(indirectLightingShader, "mapSize"), 1, (GLuint*)&mapSize);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * numChunks, voxelLightingRequests);

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
	glUniform3uiv(glGetUniformLocation(directLightingShader, "mapSize"), 1, (GLuint*)&mapSize);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * numChunks, voxelLightingRequests);

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
	glUniform3uiv(glGetUniformLocation(finalShader, "mapSize"), 1, (GLuint*)&mapSize);

	glDispatchCompute(textureSize.x / 16, textureSize.y / 16, 1);
}

void send_all_data_temp()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	uvec4* voxelMapGPU = (uvec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
	for(int z = 0; z < mapSizeGPU.z; z++)
		for(int y = 0; y < mapSizeGPU.y; y++)
			for(int x = 0; x < mapSizeGPU.x; x++)
			{
				int cpuIndex = x + mapSize.x * (y + z * mapSize.y);
				int gpuIndex = x + mapSizeGPU.x * (y + z * mapSizeGPU.y);

				voxelMapGPU[gpuIndex].x = voxelMap[cpuIndex].flag > 0 ? 2 : 0;
				voxelMapGPU[gpuIndex].y = 0;
				voxelMapGPU[gpuIndex].z = 0;
				voxelMapGPU[gpuIndex].w = 0;
			}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(VoxelMaterial) * MAX_MATERIALS, voxelMaterials);
}

//--------------------------------------------------------------------------------------------------------------------------------//

uvec2 voxel_texture_size()
{
	return textureSize;
}

bool set_voxel_texture_size(uvec2 size)
{
	return true; //TODO implement
}

uvec3 voxel_map_size()
{
	return mapSize;
}

bool set_voxel_map_size(uvec3 size)
{
	VoxelChunkHandle* newMap = malloc(sizeof(VoxelChunkHandle) * size.x * size.y * size.z);
	if(!newMap)
	{
		ERROR_LOG("ERROR - FAILED TO REALLOCATE VOXEL MAP\n");
		return false;
	}	

	int oldMaxIndex = mapSize.x * mapSize.y * mapSize.z;
	for(int z = 0; z < size.z; z++)
		for(int y = 0; y < size.y; y++)
			for(int x = 0; x < size.x; x++)
			{
				int oldIndex = x + mapSize.x * (y + z * mapSize.y);
				int newIndex = x + size.x * (y + z * size.y);

				newMap[newIndex].flag  = oldIndex < oldMaxIndex ? voxelMap[oldIndex].flag  : 0;
				newMap[newIndex].gpuIndex = oldIndex < oldMaxIndex ? voxelMap[oldIndex].gpuIndex : 0;
			}

	free(voxelMap);
	voxelMap = newMap;
	mapSize = size;
	return true;
}

unsigned int max_voxel_chunks()
{
	return maxChunks;
}

unsigned int current_voxel_chunks()
{
	return 0; //TODO implement
}

bool set_max_voxel_chunks(unsigned int num)
{
	voxelChunks = realloc(voxelChunks, sizeof(VoxelChunk) * num);
	if(!voxelChunks)
	{
		ERROR_LOG("ERROR - FAILED TO REALLOCATE VOXEL CHUNKS\n");
		return false;
	}

	maxChunks = num;
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------//

uvec3 voxel_map_size_gpu()
{
	return mapSizeGPU;
}

bool set_voxel_map_size_gpu(uvec3 size)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ivec4) * size.x * size.y * size.x, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		ERROR_LOG("ERROR - FAILED TO RESIZE VOXEL MAP BUFFER\n");
		return false;
	}

	mapSizeGPU = size;
	return true;
}

unsigned int max_voxel_chunks_gpu()
{
	return maxChunksGPU;
}

bool set_max_voxel_chunks_gpu(unsigned int num)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(VoxelChunk) * num, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		ERROR_LOG("ERROR - FAILED TO RESIZE VOXEL CHUNK BUFFER\n");
		return false;
	}

	chunkBufferLayout = realloc(chunkBufferLayout, sizeof(ivec4) * num);
	if(!chunkBufferLayout)
	{
		ERROR_LOG("ERROR - FAILED TO REALLOCATE MEMORY FOR VOXEL MATERIALS");
		return false;
	}

	if(num > maxChunksGPU)
	{
		for(int i = num - maxChunksGPU; i < num; i++)
			chunkBufferLayout[i] = (ivec4){-1, -1, -1, -1};
	}

	maxChunksGPU = num;
	return true;
}

unsigned int max_voxel_lighting_requests()
{
	return maxLightingRequests;
}

bool set_max_voxel_lighting_requests(unsigned int num)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ivec4) * num, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		ERROR_LOG("ERROR - FAILED TO RESIZE VOXEL LIGHTING REQUEST BUFFER\n");
		return false;
	}

	voxelLightingRequests = realloc(voxelLightingRequests, sizeof(ivec4) * num);
	if(!voxelLightingRequests)
	{
		ERROR_LOG("ERROR - FAILED TO REALLOCATE VOXEL LIGHTING REQUESTS\n");
		return false;
	}

	maxLightingRequests = num;
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------//

static GLuint encode_uint_RGBA(uvec4 val);
static uvec4 decode_uint_RGBA(GLuint val);

VoxelGPU voxel_to_voxelGPU(Voxel voxel)
{
	VoxelGPU res;
	uvec4 albedo = {(GLuint)(voxel.albedo.x * 255), (GLuint)(voxel.albedo.y * 255), (GLuint)(voxel.albedo.z * 255), voxel.material};
	uvec4 normal = {(GLuint)((voxel.normal.x * 0.5 + 0.5) * 255), (GLuint)((voxel.normal.y * 0.5 + 0.5) * 255), (GLuint)((voxel.normal.z * 0.5 + 0.5) * 255), 0};

	res.albedo = encode_uint_RGBA(albedo);
	res.normal = encode_uint_RGBA(normal);
	res.directLight = encode_uint_RGBA((uvec4){0, 0, 0, 1});
	res.specLight = encode_uint_RGBA((uvec4){0, 0, 0, 0});

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

VoxelChunkHandle get_map_tile(ivec3 pos) //returns the value of the map at a position DOESNT DO ANY BOUNDS CHECKING
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