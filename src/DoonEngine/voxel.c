#include "voxel.h"
#include "utility/shader.h"
#include "utility/texture.h"
#include "globals.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "math/all.h"

//--------------------------------------------------------------------------------------------------------------------------------//
//GLOBAL STATE:

GLuint lightingRequestBuffer;
GLuint materialBuffer;
GLprogram lightingProgram = 0;
GLprogram finalProgram = 0;

DNvoxelMaterial* dnVoxelMaterials = 0;

unsigned int maxLightingRequests = 1024;

const size_t halfChunkSize = sizeof(DNvoxelChunk) - sizeof(DNvec4) * 8 * 8 * 8; //CHANGE CHUNK SIZE HERE
const int WORKGROUP_SIZE = 16;

//--------------------------------------------------------------------------------------------------------------------------------//
//INITIALIZATION:

static bool _DN_gen_shader_storage_buffer(unsigned int* dest, size_t size);
static void _DN_clear_chunk(DNmap* map, unsigned int index);

bool DN_init_voxel_pipeline()
{	
	//allocate memory:
	//---------------------------------
	dnVoxelMaterials = DN_MALLOC(sizeof(DNvoxelMaterial) * DN_MAX_VOXEL_MATERIALS);
	if(!dnVoxelMaterials)
	{
		DN_ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR VOXEL MATERIALS\n");
		return false;
	}

	//generate gl buffers:
	//---------------------------------
	if(!_DN_gen_shader_storage_buffer(&materialBuffer, sizeof(DNvoxelMaterial) * DN_MAX_VOXEL_MATERIALS))
	{
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL MATERIAL BUFFER\n");
		return false;
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, materialBuffer);

	if(!_DN_gen_shader_storage_buffer(&lightingRequestBuffer, sizeof(DNuvec4) * maxLightingRequests))
	{
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE VOXEL LIGHTING REQUEST BUFFER\n");
		return false;
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightingRequestBuffer);

	//load shaders:
	//---------------------------------
	int lighting = DN_compute_program_load("shaders/voxelLighting.comp", "shaders/voxelShared.comp");
	int final    = DN_compute_program_load("shaders/voxelFinal.comp"   , "shaders/voxelShared.comp");

	if(lighting < 0 || final < 0)
	{
		DN_ERROR_LOG("ERROR - FAILED TO COMPILE 1 OR MORE VOXEL SHADERS\n");
		return false;
	}

	lightingProgram = lighting;
	finalProgram = final;

	//return:
	//---------------------------------
	return true;
}

void DN_deinit_voxel_pipeline()
{
	DN_program_free(lightingProgram);
	DN_program_free(finalProgram);

	glDeleteBuffers(1, &materialBuffer);
	glDeleteBuffers(1, &lightingRequestBuffer);

	DN_FREE(dnVoxelMaterials);
}

DNmap* DN_create_map(DNuvec3 mapSize, DNuvec2 textureSize, bool streamable, float streamIndex, unsigned int minChunks)
{
	//allocate structure:
	//---------------------------------
	DNmap* map = DN_MALLOC(sizeof(DNmap));

	//generate texture:
	//---------------------------------
	glGenTextures(1, &map->glTextureID);
	glBindTexture(GL_TEXTURE_2D, map->glTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureSize.x, textureSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//generate buffers:
	//---------------------------------
	if(!_DN_gen_shader_storage_buffer(&map->glMapBufferID, sizeof(DNvoxelChunkHandle) * mapSize.x * mapSize.y * mapSize.x))
	{
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE GPU BUFFER FOR MAP\n");
		return NULL;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, map->glMapBufferID);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

	size_t numChunks;
	if(streamable)
		numChunks = min(mapSize.x * mapSize.y * mapSize.z, minChunks);
	else
		numChunks = min(mapSize.x * mapSize.y * mapSize.z, 64); //set 64 as the minimum to allow for blank space to be ignored

	if(!_DN_gen_shader_storage_buffer(&map->glChunkBufferID, sizeof(DNvoxelChunk) * numChunks))
	{
		DN_ERROR_LOG("ERROR - FAILED TO GENERATE GPU BUFFER FOR CHUNKS\n");
		return NULL;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, map->glChunkBufferID);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL);

	//allocate CPU memory:
	//---------------------------------
	map->map = DN_MALLOC(sizeof(DNvoxelChunkHandle) * mapSize.x * mapSize.y * mapSize.z);
	if(!map->map)
	{
		DN_ERROR_LOG("ERROR - FAILED TO ALLOCATE CPU MEMORY FOR MAP\n");
		return NULL;
	}

	//set all map tiles to empty:
	for(int i = 0; i < mapSize.x * mapSize.y * mapSize.z; i++)
		map->map[i].flag = map->map[i].visible = 0;

	map->chunks = DN_MALLOC(sizeof(DNvoxelChunk) * numChunks);
	if(!map->chunks)
	{
		DN_ERROR_LOG("ERROR - FAILED TO ALLOCATE CPU MEMORY FOR CHUNKS\n");
		return NULL;
	}

	//set all voxels to empty:
	for(int i = 0; i < numChunks; i++)
		_DN_clear_chunk(map, i);

	map->lightingRequests = DN_MALLOC(sizeof(DNuvec4) * numChunks);
	if(!map->lightingRequests)
	{
		DN_ERROR_LOG("ERROR - FAILED TO ALLOCATE CPU MEMORY FOR LIGHTING REQUESTS\n");
		return NULL;
	}

	//set data parameters:
	//---------------------------------
	map->mapSize = mapSize;
	map->textureSize = textureSize;
	map->chunkCap = numChunks;
	map->nextChunk = 0;
	map->numLightingRequest = 0;
	map->lightingRequestCap = numChunks;
	map->streamable = streamable;
	map->streamIndex = streamIndex;

	//set default camera and lighting parameters:
	//---------------------------------
	map->camPos = (DNvec3){0.0f, 0.0f, 0.0f};
	map->camOrient = (DNvec3){0.0f, 0.0f, 0.0f};
	map->camFOV = 0.8f;
	map->camViewMode = 0;

	map->sunDir = (DNvec3){1.0f, 1.0f, 1.0f};
	map->sunStrength = (DNvec3){0.6f, 0.6f, 0.6f};
	map->ambientLightStrength = 0.01f; //TODO: MAKE THIS A VEC3
	map->diffuseBounceLimit = 5;
	map->specBounceLimit = 2;
	map->shadowSoftness = 10.0f;

	return map;
}

void DN_delete_map(DNmap* map)
{
	glDeleteTextures(1, &map->glTextureID);
	glDeleteBuffers(1, &map->glMapBufferID);
	glDeleteBuffers(1, &map->glChunkBufferID);

	DN_FREE(map->map);
	DN_FREE(map->chunks);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY:

unsigned int DN_add_chunk(DNmap* map, DNivec3 pos)
{
	if(!DN_in_map_bounds(map, pos))
	{
		DN_ERROR_LOG("ERROR - OUT OF BOUNDS POSITION TO REMOVE\n");
		return 0;
	}

	//search for empty chunk:
	int i = map->nextChunk;

	do
	{
		if(!map->chunks[i].pos.w) //if an empty chunk is found, use that one:
		{
			int mapIndex = DN_FLATTEN_INDEX(pos, map->mapSize);
			map->map[mapIndex].index = i;
			map->map[mapIndex].flag = 1;

			map->chunks[i].pos = (DNuvec4){pos.x, pos.y, pos.z, 1};
			map->chunks[i].updated = true;
			map->nextChunk = (i == map->chunkCap - 1) ? (0) : (i + 1);

			return i;
		}

		//jump to beginning if the end is reached:
		i++;
		if(i >= map->chunkCap)
			i = 0;

	} while(i != map->nextChunk);

	//if no empty chunk is found, increase capacity:
	size_t newCap = min(map->chunkCap * 2, map->mapSize.x * map->mapSize.y * map->mapSize.z);
	DN_ERROR_LOG("NOTE - RESIZING CHUNK MEMORY TO ALLOW FOR %zi CHUNKS\n", newCap);

	i = map->chunkCap;
	if(!DN_set_max_chunks(map, newCap))
		return 0;

	//clear new chunks:
	for(int j = i; j < map->chunkCap; j++)
		_DN_clear_chunk(map, j);

	int mapIndex = DN_FLATTEN_INDEX(pos, map->mapSize);
	map->map[mapIndex].index = i;
	map->map[mapIndex].flag = 1;

	map->chunks[i].pos = (DNuvec4){pos.x, pos.y, pos.z, 1};
	map->chunks[i].updated = true;
	map->nextChunk = (i == map->chunkCap - 1) ? (0) : (i + 1);

	return i;
}

void DN_remove_chunk(DNmap* map, DNivec3 pos)
{
	if(!DN_in_map_bounds(map, pos))
	{
		DN_ERROR_LOG("ERROR - OUT OF BOUNDS POSITION TO REMOVE\n");
		return;
	}

	int mapIndex = DN_FLATTEN_INDEX(pos, map->mapSize);
	map->map[mapIndex].flag = 0;
	map->nextChunk = map->map[mapIndex].index;
	_DN_clear_chunk(map, map->map[mapIndex].index);
}

static void _DN_sync_gpu_streamable(DNmap* map, DNmemOp op, DNchunkRequests requests)
{

}

static void _DN_sync_gpu_nonstreamable(DNmap* map, DNmemOp op, DNchunkRequests requests)
{
	if(op != DN_WRITE)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, map->glMapBufferID);
		DNvoxelChunkHandle* gpuMap = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

		if(requests != DN_REQUEST_NONE)
			map->numLightingRequest = 0;

		unsigned int maxIndex = map->mapSize.x * map->mapSize.y * map->mapSize.z;
		for(int i = 0; i < maxIndex; i++)
		{
			DNvoxelChunkHandle cell = (op == DN_READ_WRITE) ? map->map[i] : gpuMap[i]; //if we are going to upload a new map, use data from that new map instead of old data

			if(requests != DN_REQUEST_NONE && cell.flag == 1 && (requests == DN_REQUEST_LOADED || cell.visible == 1))
			{
				if(map->numLightingRequest >= map->lightingRequestCap)
				{
					size_t newCap = min(maxIndex, map->lightingRequestCap * 2);
					DN_ERROR_LOG("NOTE - RESIZING LIGHTING REQUEST MEMORY TO ALLOW FOR %zi REQUESTS\n", newCap);
					if(!DN_set_max_lighting_requests(map, newCap))
						break;
				}

				map->lightingRequests[map->numLightingRequest++].x = cell.index;
			}

			gpuMap[i].visible = 0;
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	if(op != DN_READ)
	{
		//send entire map:
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, map->glMapBufferID);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, map->mapSize.x * map->mapSize.y * map->mapSize.z * sizeof(DNvoxelChunkHandle), map->map);

		//send chunks:
		GLint bufferSize;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, map->glChunkBufferID);
		glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &bufferSize);

		if(bufferSize == map->chunkCap * sizeof(DNvoxelChunk)) //if chunk buffer was not resized, only send the updated chunks:
		{
			for(int i = 0; i < map->chunkCap; i++)
			{
				DNvoxelChunk chunk = map->chunks[i];
				if(chunk.pos.w == 1 && chunk.updated)
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sizeof(DNvoxelChunk), halfChunkSize, &map->chunks[i]);
			}
		}
		else //resize and reupload buffer if a new size is needed:
		{
			glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL); //clear to 0 so that baked lighting gets reset
			glBufferData(GL_SHADER_STORAGE_BUFFER, map->chunkCap * sizeof(DNvoxelChunk), map->chunks, GL_DYNAMIC_DRAW);
		}
	}
}

void DN_sync_gpu(DNmap* map, DNmemOp op, DNchunkRequests requests)
{
	if(map->streamable)
		_DN_sync_gpu_streamable(map, op, requests);
	else
		_DN_sync_gpu_nonstreamable(map, op, requests);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//UPDATING/DRAWING:

/*static void _DN_stream_voxel_chunk(DNivec3 pos, DNvoxelChunkHandle* voxelMapGPU, int gpuIndex, int cpuIndex, bool autoResize)
{
	const int maxGpuMapIndex = mapSizeGPU.x * mapSizeGPU.y * mapSizeGPU.z; //the maximum map index on the GPU

	//variables that belong to the oldest chunk:
	int maxTime = 0;
	int maxTimeIndex = -1;
	int maxTimeMapIndex = -1;

	//loop through every chunk to look for oldest:
	for(int i = 0; i < maxChunksGPU; i++)
	{
		DNivec4 chunkPos = chunkBufferLayout[i];
		int chunkMapIndex = DN_FLATTEN_INDEX(chunkPos, mapSizeGPU);

		//if chunk does not belong to any active map tile, instantly use it
		if(chunkPos.w < 0 || chunkMapIndex >= maxGpuMapIndex)
		{
			maxTime = 2;
			maxTimeIndex = i;
			maxTimeMapIndex = -1;
			break;
		}
		else if(voxelMapGPU[chunkMapIndex].lastUsed > maxTime) //else, check if it is older than the current oldest
		{
			maxTime = voxelMapGPU[chunkMapIndex].lastUsed;
			maxTimeIndex = i;
			maxTimeMapIndex = chunkMapIndex;
		}
	}

	if(maxTime > 1) //only load in the new chunk if the old one isnt currently in use
	{
		if(maxTimeMapIndex >= 0) //if the chunk was previously loaded, set its flag to unloaded and store old data (to maintain semi-baked lighting)
		{
			voxelMapGPU[maxTimeMapIndex].flag = voxelMapGPU[maxTimeMapIndex].flag > 0 ? 2 : 0;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, maxTimeIndex * sizeof(DNvoxelChunk) + halfChunkSize, halfChunkSize, &dnVoxelChunks[maxTimeMapIndex].indirectLight[0][0][0]);
		}

		voxelMapGPU[gpuIndex].flag = 1; //set the new tile's flag to loaded
		voxelMapGPU[gpuIndex].lastUsed = 0;
		voxelMapGPU[gpuIndex].index = maxTimeIndex;

		//load in new data:
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, maxTimeIndex * sizeof(DNvoxelChunk), sizeof(DNvoxelChunk), &dnVoxelChunks[cpuIndex]);

		chunkBufferLayout[maxTimeIndex] = (DNivec4){pos.x, pos.y, pos.z, 0}; //update the chunk layout
	}
	else if(autoResize)//if the oldest chunk is currently in use, double the buffer size
	{
		DN_ERROR_LOG("NOTE - MAXIMUM GPU CHUNK LIMIT REACHED... RESIZING TO %i CHUNKS\n", maxChunksGPU * 2);

		//create a temporary buffer to store the old chunk data (opengl doesnt have a "realloc" function)
		void* oldChunkData = DN_MALLOC(sizeof(DNvoxelChunk) * maxChunksGPU);
		if(!oldChunkData)
		{
			DN_ERROR_LOG("ERROR - FAILED TO ALLOCATE MEMORY FOR TEMPORARY CHUNK STORAGE\n");
			return;
		}

		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DNvoxelChunk) * maxChunksGPU, oldChunkData); //get old data
		DN_set_max_voxel_chunks_gpu(maxChunksGPU * 2); //resize buffer
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DNvoxelChunk) * (maxChunksGPU / 2), oldChunkData); //send back old data

		DN_FREE(oldChunkData); //free temporary memory
	}
	else
		DN_ERROR_LOG("NOTE - MAXIMUM GPU CHUNK LIMIT REACHED... CHUNKS MAY NOT BE VISIBLE\n");
}*/

/*unsigned int DN_stream_voxel_chunks(bool updateLighting, bool autoResize)
{
	//map the buffer:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	DNvoxelChunkHandle* voxelMapGPU = (DNvoxelChunkHandle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	//bind the chunk buffer so it can be updated:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);

	//the current number of chunks to have their lighting updated:
	int numChunksToUpdate = 0;

	//loop through every map tile:
	for(int z = 0; z < mapSizeGPU.z; z++)
	for(int y = 0; y < mapSizeGPU.y; y++)
	for(int x = 0; x < mapSizeGPU.x; x++)
	{
		DNivec3 pos = {x, y, z};
		int cpuMapIndex = DN_FLATTEN_INDEX(pos, mapSize);    //the map index for the cpu-side memory
		int gpuMapIndex = DN_FLATTEN_INDEX(pos, mapSizeGPU); //the map index for the gpu-side memory

		//if a chunk was added to the cpu map, request it to be added to the gpu map:
		if(dnVoxelMap[cpuMapIndex].flag > 0 && voxelMapGPU[gpuMapIndex].flag == 0)
			voxelMapGPU[gpuMapIndex].flag = 3;

		//if flag = 3 (requested), try to load a new chunk:
		if(voxelMapGPU[gpuMapIndex].flag == 3 && dnVoxelMap[cpuMapIndex].flag == 1)
			_DN_stream_voxel_chunk(pos, voxelMapGPU, gpuMapIndex, cpuMapIndex, autoResize);

		//if chunk is loaded and visible, add to lighting request buffer:
		if(updateLighting && voxelMapGPU[gpuMapIndex].flag == 1 && voxelMapGPU[gpuMapIndex].visible == 1)
		{
			if(numChunksToUpdate <= maxLightingRequests)
				dnVoxelLightingRequests[numChunksToUpdate++] = (DNivec4){x, y, z, 0};
			else if(autoResize)
			{
				DN_ERROR_LOG("NOTE - NUMBER OF VISIBLE CHUNKS EXCEEDS MAX LIGHTING UPDATES... RESIZING LIGHTING REQUESTS TO ACCOMADATE %i CHUNKS\n", maxLightingRequests * 2);
				DN_set_max_voxel_lighting_requests(2 * maxLightingRequests);
			}
			else
				DN_ERROR_LOG("NOTE - NUMBER OF VISIBLE CHUNKS EXCEEDS MAX LIGHTING UPDATES... UNABLE TO UPDATE ALL CHUNKS\n");
		}

		//increase the "time last used" flag:
		if(voxelMapGPU[gpuMapIndex].lastUsed < UINT32_MAX)
			voxelMapGPU[gpuMapIndex].lastUsed++;

 		//set the "visible" flag to 0:
		voxelMapGPU[gpuMapIndex].visible = 0;
	}

	//unmap:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	return numChunksToUpdate;
}*/

/*void DN_update_voxel_chunk(DNivec3* positions, unsigned int num, bool updateLighting, DNvec3 camPos, float time)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer);

	for(int i = 0; i < num; i++)
		for(int j = 0; j < maxChunksGPU; j++)
		{
			DNivec3 pos = positions[i];
			DNivec4 bufferPos = chunkBufferLayout[j];
			if(bufferPos.x == pos.x && bufferPos.y == pos.y && bufferPos.z == pos.z)
			{
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, j * sizeof(DNvoxelChunk), halfChunkSize, &dnVoxelChunks[DN_FLATTEN_INDEX(pos, mapSize)]);
				dnVoxelLightingRequests[i] = (DNivec4){pos.x, pos.y, pos.z, 0};
				break;
			}
		}

	if(updateLighting)
		DN_update_voxel_lighting(num, 0, camPos, time);
}*/

void DN_draw_voxels(DNmap* map)
{
	float aspectRatio = (float)map->textureSize.y / (float)map->textureSize.x;
	DNmat3 rotate = DN_mat4_to_mat3(DN_mat4_rotate_euler(DN_MAT4_IDENTITY, map->camOrient));
	DNvec3 camFront = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, map->camFOV });
	DNvec3 camPlaneU;
	DNvec3 camPlaneV;

	if(aspectRatio < 1.0f)
	{
		camPlaneU = DN_mat3_mult_vec3(rotate, (DNvec3){-1.0f, 0.0f, 0.0f});
		camPlaneV = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 1.0f * aspectRatio, 0.0f});
	}
	else
	{
		camPlaneU = DN_mat3_mult_vec3(rotate, (DNvec3){-1.0f / aspectRatio, 0.0f, 0.0f});
		camPlaneV = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 1.0f, 0.0f});
	}

	glUseProgram(finalProgram);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, map->glChunkBufferID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, map->glMapBufferID);
	glBindImageTexture(0, map->glTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	DN_program_uniform_vec3 (finalProgram, "camPos", map->camPos);
	DN_program_uniform_vec3 (finalProgram, "camDir", camFront);
	DN_program_uniform_vec3 (finalProgram, "camPlaneU", camPlaneU);
	DN_program_uniform_vec3 (finalProgram, "camPlaneV", camPlaneV);
	DN_program_uniform_vec3 (finalProgram, "sunStrength", map->sunStrength);
	DN_program_uniform_uint (finalProgram, "viewMode", map->camViewMode);
	DN_program_uniform_float(finalProgram, "ambientStrength", map->ambientLightStrength);
	glUniform3uiv(glGetUniformLocation(finalProgram, "mapSize"), 1, (GLuint*)&map->mapSize);

	glBindImageTexture(0, map->glTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glDispatchCompute(map->textureSize.x / WORKGROUP_SIZE, map->textureSize.y / WORKGROUP_SIZE, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void DN_update_voxel_lighting(DNmap* map, unsigned int offset, unsigned int num, float time)
{
	/*if(numChunks > maxLightingRequests)
	{
		DN_ERROR_LOG("WARNING - NUM CHUNKS REQUESTED TO BE UPDATED IS GREATER THAN THE MAXIMUM, UPDATING THE MAX NUMBER INSTEAD\n");
		numChunks = maxLightingRequests;
	}

	DN_program_activate(lightingProgram);
	DN_program_uniform_vec3 (lightingProgram, "camPos", camPos);
	DN_program_uniform_float(lightingProgram, "time", time);

	glUniform3uiv(glGetUniformLocation(lightingProgram, "mapSize"), 1, (GLuint*)&mapSize);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DNivec4) * numChunks, &dnVoxelLightingRequests[offset]);

	glDispatchCompute(numChunks, 1, 1);*/

	glUseProgram(lightingProgram);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, map->glChunkBufferID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, map->glMapBufferID);
	num = num == 0 ? map->numLightingRequest : num;
	if(offset + num > map->lightingRequestCap)
		num = map->lightingRequestCap - offset;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer);

	if(num > maxLightingRequests)
	{
		size_t newSize = maxLightingRequests;
		while(newSize < num)
			newSize *= 2;

		DN_ERROR_LOG("NOTE - RESIZING LIGHTING REQUESTS BUFFER TO ACCOMODATE %zi REQUESTS\n", newSize);
		glBufferData(GL_SHADER_STORAGE_BUFFER, newSize * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		if(glGetError() == GL_OUT_OF_MEMORY)
		{
			DN_ERROR_LOG("ERROR - FAILED TO RESIZE LIGHTING REQUESTS BUFFER\n");
			return;
		}
		maxLightingRequests = newSize;
	}

	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num * sizeof(DNuvec4), &map->lightingRequests[offset]);

	DN_program_uniform_vec3(lightingProgram, "camPos", map->camPos);
	DN_program_uniform_float(lightingProgram, "time", time);
	DN_program_uniform_uint(lightingProgram, "diffuseBounceLimit", map->diffuseBounceLimit);
	DN_program_uniform_uint(lightingProgram, "specularBounceLimit", map->specBounceLimit);
	DN_program_uniform_vec3(lightingProgram, "sunDir", DN_vec3_normalize(map->sunDir));
	DN_program_uniform_vec3(lightingProgram, "sunStrength", map->sunStrength);
	DN_program_uniform_float(lightingProgram, "shadowSoftness", map->shadowSoftness);
	DN_program_uniform_float(lightingProgram, "ambientStrength", map->ambientLightStrength);
	glUniform3uiv(glGetUniformLocation(lightingProgram, "mapSize"), 1, (GLuint*)&map->mapSize);

	glDispatchCompute(num, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//CPU-SIDE MAP SETTINGS:

bool DN_set_max_chunks(DNmap* map, unsigned int num)
{
	DNvoxelChunk* newChunks = DN_REALLOC(map->chunks, sizeof(DNvoxelChunk) * num);

	if(!newChunks)
	{
		DN_ERROR_LOG("ERROR - UNABLE TO RESIZE CHUNK MEMORY\n");
		return false;
	}

	map->chunks = newChunks;
	map->chunkCap = num;
	return true;
}

bool DN_set_max_lighting_requests(DNmap* map, unsigned int num)
{
	DNuvec4* newRequests = DN_REALLOC(map->lightingRequests, sizeof(DNuvec4) * num);

	if(!newRequests)
	{
		DN_ERROR_LOG("ERROR - UNABLE TO RESIZE LIGHTING REQUEST MEMORY\n");
		return false;
	}

	map->lightingRequests = newRequests;
	map->lightingRequestCap = num;
	return true;
}

/*DNuvec2 DN_voxel_texture_size()
{
	return textureSize;
}

bool DN_set_voxel_texture_size(DNuvec2 size)
{
	size.x += WORKGROUP_SIZE - size.x % WORKGROUP_SIZE;
	size.y += WORKGROUP_SIZE - size.y % WORKGROUP_SIZE;

	DN_texture_bind(finalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		DN_ERROR_LOG("ERROR - FAILED TO RESIZE FINAL TEXTURE\n");
		return false;
	}

	textureSize = size;
	return true;
}

DNuvec3 DN_voxel_map_size()
{
	return mapSize;
}

bool DN_set_voxel_map_size(DNuvec3 size)
{
	DNvoxelChunkHandle* newMap = DN_MALLOC(sizeof(DNvoxelChunkHandle) * size.x * size.y * size.z);
	if(!newMap)
	{
		DN_ERROR_LOG("ERROR - FAILED TO REALLOCATE VOXEL MAP\n");
		return false;
	}	

	int oldMaxIndex = mapSize.x * mapSize.y * mapSize.z;
	for(int z = 0; z < size.z; z++)
		for(int y = 0; y < size.y; y++)
			for(int x = 0; x < size.x; x++)
			{
				int oldIndex = x + mapSize.x * (y + z * mapSize.y);
				int newIndex = x + size.x * (y + z * size.y);

				newMap[newIndex].flag  = oldIndex < oldMaxIndex ? dnVoxelMap[oldIndex].flag  : 0;
			}

	DN_FREE(dnVoxelMap);
	dnVoxelMap = newMap;
	mapSize = size;
	return true;
}

unsigned int DN_max_voxel_chunks()
{
	return maxChunks;
}

unsigned int DN_current_voxel_chunks()
{
	return 0; //TODO implement
}

bool DN_set_max_voxel_chunks(unsigned int num)
{
	dnVoxelChunks = DN_REALLOC(dnVoxelChunks, sizeof(DNvoxelChunk) * num);
	if(!dnVoxelChunks)
	{
		DN_ERROR_LOG("ERROR - FAILED TO REALLOCATE VOXEL CHUNKS\n");
		return false;
	}

	maxChunks = num;
	return true;
}*/

//--------------------------------------------------------------------------------------------------------------------------------//
//GPU-SIDE MAP SETTINGS:

/*DNuvec3 DN_voxel_map_size_gpu()
{
	return mapSizeGPU;
}

bool DN_set_voxel_map_size_gpu(DNuvec3 size)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DNivec4) * size.x * size.y * size.x, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		DN_ERROR_LOG("ERROR - FAILED TO RESIZE VOXEL MAP BUFFER\n");
		return false;
	}

	mapSizeGPU = size;
	return true;
}

unsigned int DN_max_voxel_chunks_gpu()
{
	return maxChunksGPU;
}

bool DN_set_max_voxel_chunks_gpu(unsigned int num)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DNvoxelChunk) * num, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		DN_ERROR_LOG("ERROR - FAILED TO RESIZE VOXEL CHUNK BUFFER\n");
		return false;
	}

	chunkBufferLayout = DN_REALLOC(chunkBufferLayout, sizeof(DNivec4) * num);
	if(!chunkBufferLayout)
	{
		DN_ERROR_LOG("ERROR - FAILED TO REALLOCATE MEMORY FOR VOXEL MATERIALS");
		return false;
	}

	if(num > maxChunksGPU)
	{
		for(int i = num - maxChunksGPU; i < num; i++)
			chunkBufferLayout[i] = (DNivec4){-1, -1, -1, -1};
	}

	maxChunksGPU = num;
	return true;
}

unsigned int DN_max_voxel_lighting_requests()
{
	return maxLightingRequests;
}

bool DN_set_max_voxel_lighting_requests(unsigned int num)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightingRequestBuffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DNivec4) * num, NULL, GL_DYNAMIC_DRAW); //allocate
	if(glGetError() == GL_OUT_OF_MEMORY)
	{	
		DN_ERROR_LOG("ERROR - FAILED TO RESIZE VOXEL LIGHTING REQUEST BUFFER\n");
		return false;
	}

	dnVoxelLightingRequests = DN_REALLOC(dnVoxelLightingRequests, sizeof(DNivec4) * num);
	if(!dnVoxelLightingRequests)
	{
		DN_ERROR_LOG("ERROR - FAILED TO REALLOCATE VOXEL LIGHTING REQUESTS\n");
		return false;
	}

	maxLightingRequests = num;
	return true;
}

void DN_set_voxel_lighting_parameters(DNvec3 sunDir, DNvec3 sunStrength, float ambientLightStrength, unsigned int diffuseBounceLimit, unsigned int specBounceLimit, float shadowSoftness)
{
	DN_program_activate(lightingProgram);
	DN_program_uniform_vec3 (lightingProgram, "sunDir", DN_vec3_normalize(sunDir));
	DN_program_uniform_vec3 (lightingProgram, "sunStrength", sunStrength);
	DN_program_uniform_float(lightingProgram, "ambientStrength", ambientLightStrength);
	DN_program_uniform_uint (lightingProgram, "diffuseBounceLimit", diffuseBounceLimit);
	DN_program_uniform_uint (lightingProgram, "specularBounceLimit", specBounceLimit);
	DN_program_uniform_float(lightingProgram, "shadowSoftness", shadowSoftness);

	DN_program_activate(finalProgram);
	DN_program_uniform_float(finalProgram, "ambientStrength", ambientLightStrength);
	DN_program_uniform_vec3 (finalProgram, "sunStrength", sunStrength);
}*/

void DN_set_voxel_materials(DNmaterialHandle min, DNmaterialHandle max)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DNvoxelMaterial) * (max - min + 1), &dnVoxelMaterials[min]);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

static GLuint _DN_encode_uint_RGBA(DNuvec4 val);
static DNuvec4 _DN_decode_uint_RGBA(GLuint val);

DNvoxelGPU DN_voxel_to_voxelGPU(DNvoxel voxel)
{
	DNvoxelGPU res;
	voxel.albedo = DN_vec3_clamp(voxel.albedo, 0.0f, 1.0f);
	voxel.normal = DN_vec3_clamp(voxel.normal, -1.0f, 1.0f);
	DNuvec4 albedo = {(GLuint)(voxel.albedo.x * 255), (GLuint)(voxel.albedo.y * 255), (GLuint)(voxel.albedo.z * 255), voxel.material};
	DNuvec4 normal = {(GLuint)((voxel.normal.x * 0.5 + 0.5) * 255), (GLuint)((voxel.normal.y * 0.5 + 0.5) * 255), (GLuint)((voxel.normal.z * 0.5 + 0.5) * 255), voxel.mask & 0xFF};

	res.albedo = _DN_encode_uint_RGBA(albedo);
	res.normal = _DN_encode_uint_RGBA(normal);
	res.directLight = _DN_encode_uint_RGBA((DNuvec4){0, 0, 0, (voxel.mask >> 8) & 0xFF});
	res.specLight = _DN_encode_uint_RGBA((DNuvec4){0, 0, 0, 0});

	return res;
}

DNvoxel DN_voxelGPU_to_voxel(DNvoxelGPU voxel)
{
	DNvoxel res;
	DNuvec4 albedo = _DN_decode_uint_RGBA(voxel.albedo);
	DNuvec4 normal = _DN_decode_uint_RGBA(voxel.normal);
	DNuvec4 directLight = _DN_decode_uint_RGBA(voxel.directLight);

	res.albedo = DN_vec3_scale((DNvec3){albedo.x, albedo.y, albedo.z}, 0.00392156862);
	DNvec3 scaledNormal = DN_vec3_scale((DNvec3){normal.x, normal.y, normal.z}, 0.00392156862);
	res.normal = (DNvec3){(scaledNormal.x - 0.5) * 2.0, (scaledNormal.y - 0.5) * 2.0, (scaledNormal.z - 0.5) * 2.0};
	res.material = albedo.w;
	res.mask = normal.w | (directLight.w << 8);

	return res;
}

//--------------------------------------------------------------------------------------------------------------------------------//

bool DN_in_map_bounds(DNmap* map, DNivec3 pos) //returns whether a position is in bounds in the map
{
	return pos.x < map->mapSize.x && pos.y < map->mapSize.y && pos.z < map->mapSize.z && pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

bool DN_in_chunk_bounds(DNivec3 pos) //returns whether a position is in bounds in a chunk
{
	return pos.x < DN_CHUNK_SIZE.x && pos.y < DN_CHUNK_SIZE.y && pos.z < DN_CHUNK_SIZE.z && pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

/*DNvoxelChunkHandle DN_get_voxel_chunk_handle(DNivec3 pos) //returns the value of the map at a position DOESNT DO ANY BOUNDS CHECKING
{
	return dnVoxelMap[DN_FLATTEN_INDEX(pos, mapSize)];
}

DNvoxelGPU DN_get_voxel(unsigned int chunk, DNivec3 pos) //returns the voxel of a chunk at a position DOESNT DO ANY BOUNDS CHECKING
{
 	return dnVoxelChunks[chunk].voxels[pos.x][pos.y][pos.z];
}

bool DN_does_voxel_exist(unsigned int chunk, DNivec3 localPos) //returns true if the voxel at the position is solid (not empty)
{
	DNvoxelGPU voxel = DN_get_voxel(chunk, localPos);
	unsigned int material = voxel.albedo & 0xFF;
	return material < 255;
}

static int sign(float num)
{
	return (num > 0.0f) ? 1 : ((num < 0.0f) ? -1 : 0);
}

bool DN_step_voxel_map(DNvec3 rayDir, DNvec3 rayPos, unsigned int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal)
{
	*hitNormal = (DNivec3){-1000, -1000, -1000};

	//scale to use voxel-level coordinates
	rayPos = DN_vec3_scale(rayPos, DN_CHUNK_SIZE.x);

	//utility:
	DNvec3 invRayDir = {1 / rayDir.x, 1 / rayDir.y, 1 / rayDir.z};
	DNvec3 rayDirSign = {sign(rayDir.x), sign(rayDir.y), sign(rayDir.z)};

	//create vars needed for dda:
	DNivec3 pos = {floor(rayPos.x), floor(rayPos.y), floor(rayPos.z)}; //the position in the voxel map
	DNvec3 deltaDist = {fabsf(invRayDir.x), fabsf(invRayDir.y), fabsf(invRayDir.z)}; //the distance the ray has to travel to move one unit in each direction
	DNivec3 rayStep = {rayDirSign.x, rayDirSign.y, rayDirSign.z}; //the direction the ray steps

	DNvec3 sideDist; //the total distance the ray has to travel to reach one additional unit in each direction (accounts for starting position as well)
	sideDist.x = (rayDirSign.x * (pos.x - rayPos.x) + (rayDirSign.x * 0.5) + 0.5) * deltaDist.x;
	sideDist.y = (rayDirSign.y * (pos.y - rayPos.y) + (rayDirSign.y * 0.5) + 0.5) * deltaDist.y;
	sideDist.z = (rayDirSign.z * (pos.z - rayPos.z) + (rayDirSign.z * 0.5) + 0.5) * deltaDist.z;

	unsigned int numSteps = 0;
	while(numSteps < maxSteps)
	{
		//check if in bounds:
		DNivec3 mapPos = {pos.x / DN_CHUNK_SIZE.x, pos.y / DN_CHUNK_SIZE.y, pos.z / DN_CHUNK_SIZE.z};
		if(pos.x < 0)
			mapPos.x--;
		if(pos.y < 0)
			mapPos.y--;
		if(pos.z < 0)
			mapPos.z--;

		if(DN_in_voxel_map_bounds(mapPos))
		{
			DNvoxelChunkHandle mapTile = DN_get_voxel_chunk_handle(mapPos);
			DNivec3 localPos = {pos.x % DN_CHUNK_SIZE.x, pos.y % DN_CHUNK_SIZE.y, pos.z % DN_CHUNK_SIZE.z};

			//check if voxel exists:
			if(mapTile.flag == 1 && DN_does_voxel_exist(mapTile.index, localPos))
			{
				*hitVoxel = DN_voxelGPU_to_voxel(DN_get_voxel(mapTile.index, localPos));
				*hitPos = pos;
				return true;
			}
		}

		//iterate DDA algorithm:
		if (sideDist.x < sideDist.y) 
			if (sideDist.x < sideDist.z) 
			{
				sideDist.x += deltaDist.x;
				pos.x += rayStep.x;
				*hitNormal = (DNivec3){-rayStep.x, 0, 0};
			}
			else 
			{
				sideDist.z += deltaDist.z;
				pos.z += rayStep.z;
				*hitNormal = (DNivec3){0, 0, -rayStep.z};
			}
		else
			if (sideDist.y < sideDist.z) 
			{
				sideDist.y += deltaDist.y;
				pos.y += rayStep.y;
				*hitNormal = (DNivec3){0, -rayStep.y, 0};
			}
			else 
			{
				sideDist.z += deltaDist.z;
				pos.z += rayStep.z;
				*hitNormal = (DNivec3){0, 0, -rayStep.z};
			}

		numSteps++;
	}

	return false;
}*/

//--------------------------------------------------------------------------------------------------------------------------------//
//STATIC UTIL FUNCTIONS:

static bool _DN_gen_shader_storage_buffer(unsigned int* dest, size_t size)
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

	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer); //bind to 0th index buffer binding, also defined in the shader

	*dest = buffer;

	return true;
}

static void _DN_clear_chunk(DNmap* map, unsigned int index)
{
	map->chunks[index].pos = (DNuvec4){0, 0, 0, 0};
	map->chunks[index].updated = false;
	map->chunks[index].numVoxels = 0;

	for(int z = 0; z < DN_CHUNK_SIZE.z; z++)
		for(int y = 0; y < DN_CHUNK_SIZE.y; y++)
			for(int x = 0; x < DN_CHUNK_SIZE.x; x++)
			{
				map->chunks[index].voxels[x][y][z].albedo = UINT32_MAX;
				map->chunks[index].indirectLight[x][y][z] = (DNvec4){0, 0, 0, 0};
			}
}

static GLuint _DN_encode_uint_RGBA(DNuvec4 val)
{
	val.x = val.x & 0xFF;
	val.y = val.y & 0xFF;
	val.z = val.z & 0xFF;
	val.w = val.w & 0xFF;
	return val.x << 24 | val.y << 16 | val.z << 8 | val.w;
}

static DNuvec4 _DN_decode_uint_RGBA(GLuint val)
{
	DNuvec4 res;
	res.x = (val >> 24) & 0xFF;
	res.y = (val >> 16) & 0xFF;
	res.z = (val >> 8) & 0xFF;
	res.w = (val) & 0xFF;
	return res;
}