#include "voxel.h"
#include "utility/shader.h"
#include "globals.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//GPU STRUCTS:

//a single voxel, as stored on the GPU
typedef struct DNvoxelGPU
{
	GLuint normal;       //layout: material index (8 bits) | normal.x (8 bits) | normal.y (8 bits) | normal.z (8 bits)
	GLuint directLight;  //used to store how much direct light the voxel receives,   not updated CPU-side
	GLuint specLight;    //used to store how much specular light the voxel receives, not updated CPU-side
	GLuint diffuseLight; //used to store how much diffuse light the voxel receives,  not updated CPU-side
} DNvoxelGPU;

//a chunk of voxels, as stored on the GPU
typedef struct DNchunkGPU
{
	DNivec3 pos;               //this chunk's position within the map
	GLuint numLightingSamples; //the number of lighting samples this chunk has taken, not updated CPU-side

	GLuint partialCounts[3];   //how many voxels exist in this chunk before the 1/4, 1/2, and 3/4 positions, respectively. used to accelerate finding a voxel
	GLuint bitMask[16];        //a bit mask where each bit represents whether a voxel is present or not

	GLuint padding;            //for gpu alignment
} DNchunkGPU;

//a handle to a voxel chunk, as stored on the GPU
typedef struct DNchunkHandleGPU
{
	GLuint flags;      //layout: chunk index (28 bits) | visible flag (2 bits) | loaded flag (2 bits)
	GLuint lastUsed;   //the time, in frames, since the chunk was last used
	GLuint voxelIndex; //the index to the voxel data for the chunk that this handle points to
} DNchunkHandleGPU;

//--------------------------------------------------------------------------------------------------------------------------------//
//HELPER FUNCTIONS:

//gl error checking:

//clears all gl errors
static void _DN_clear_gl_errors();
//checks if any gl errors occured
static bool _DN_gl_error();

//intialization:

//generates a shader storage buffer and places the handle into dest
static bool _DN_gen_shader_storage_buffer(GLuint* dest, size_t size);
//clears a chunk, settting all voxels to empty
static void _DN_clear_chunk(DNvolume* vol, int index);

//file i/o and compression:

//byte vec3
typedef struct DNbvec3
{
	uint8_t x, y, z;
} DNbvec3;

//writes data to a buffer
static void _DN_write_buffer(char** dest, void* src, size_t size);
//reads data from a buffer
static void _DN_read_buffer(void* dest, char** src, size_t size);

//finds an item in a palette and returns the index, returns -1 if item wasn't found
static int _DN_find_in_palette(DNbvec3* palette, int paletteSize, DNbvec3 item);

//cpu/gpu streaming:

//returns whether a voxel's face is visible
static bool _DN_check_face_visible(DNvolume* vol, DNchunk chunk, DNivec3 pos);
//converts a DNchunk to a DNchunkGPU
static DNchunkGPU _DN_chunk_to_gpu(DNvolume* vol, DNchunk chunk, int* numVoxels, DNvoxelGPU* voxels);

//determines if a chunk should have its lighting updated, if so, adds it to the request buffer
static void _DN_request_chunk_lighting(DNvolume* vol, DNchunkHandle* cpuMap, int mapIndex, int gpuFlag, int gpuChunkIndex, bool gpuVisible, int lightingSplit);
//streams chunk and voxel data to the gpu for a given chunk
static void _DN_stream_to_gpu(DNvolume* vol, DNchunkHandle* cpuMap, DNchunkHandleGPU* gpuMap, DNivec3 pos, int mapIndex, int* gpuFlag, int gpuChunkIndex, bool* resizeVoxels);

//unloads a chunk gpu-side
static void _DN_unload_voxels(DNvolume* vol, int mapIndex, int chunkIndex);
//streams in a chunk (without the voxel data)
static void _DN_stream_chunk(DNvolume* vol, int mapIndex, DNchunkGPU chunk);
//streams in voxel data, returns true if buffer needs to be resized, false otherwise
static bool _DN_stream_voxels(DNvolume* vol, DNivec3 pos, DNchunkHandleGPU* mapGPU, int mapIndex, int numVoxels, DNvoxelGPU* voxels);

//sorts the gpu voxel buffer to combine adjacent nodes into larger ones
static void _DN_sort_gpu_voxel_buffer(DNvolume* vol, DNchunkHandleGPU* gpuMap, int maxCopies);

//--------------------------------------------------------------------------------------------------------------------------------//
//GLOBAL STATE:

void (*g_DN_message_callback)(DNmessageType, DNmessageSeverity, const char*);

GLuint g_lightingRequestBuffer = 0;
GLuint g_materialBuffer        = 0;
GLprogram g_lightingProgram = 0;
GLprogram g_drawProgram     = 0;

int g_maxLightingRequests = 1024;

#define DRAW_WORKGROUP_SIZE 16
#define LIGHTING_WORKGROUP_SIZE 32

#define GET_MATERIAL_ID(x) ((x) >> 24)

//--------------------------------------------------------------------------------------------------------------------------------//
//INITIALIZATION:

bool DN_init()
{	
	//generate gl buffers:
	//---------------------------------
	if(!_DN_gen_shader_storage_buffer(&g_materialBuffer, sizeof(DNmaterial) * DN_MAX_MATERIALS))
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_FATAL, "failed to generate material buffer");
		return false;
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_materialBuffer);

	if(!_DN_gen_shader_storage_buffer(&g_lightingRequestBuffer, sizeof(GLuint) * g_maxLightingRequests))
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_FATAL, "failed to generate lighting request buffer");
		return false;
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_lightingRequestBuffer);

	//load shaders:
	//---------------------------------
	int lighting = DN_compute_program_load("shaders/voxelLighting.comp", "shaders/voxelShared.comp");
	int draw     = DN_compute_program_load("shaders/voxelDraw.comp"    , "shaders/voxelShared.comp");

	if(lighting < 0 || draw < 0)
	{
		g_DN_message_callback(DN_MESSAGE_SHADER, DN_MESSAGE_FATAL, "failed to compile 1 or more voxel shaders");
		return false;
	}

	g_lightingProgram = lighting;
	g_drawProgram = draw;

	//return:
	//---------------------------------
	return true;
}

void DN_quit()
{
	DN_program_free(g_lightingProgram);
	DN_program_free(g_drawProgram);

	glDeleteBuffers(1, &g_materialBuffer);
	glDeleteBuffers(1, &g_lightingRequestBuffer);
}

DNvolume* DN_create_volume(DNuvec3 mapSize, unsigned int minChunks)
{
	//allocate structure:
	//---------------------------------
	DNvolume* vol = DN_MALLOC(sizeof(DNvolume));

	//generate buffers:
	//---------------------------------
	if(!_DN_gen_shader_storage_buffer(&vol->glMapBufferID, sizeof(DNchunkHandleGPU) * mapSize.x * mapSize.y * mapSize.x))
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_FATAL, "failed to generate map buffer");
		return NULL;
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glMapBufferID);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, NULL);

	size_t numChunks;
	numChunks = fmin(mapSize.x * mapSize.y * mapSize.z, minChunks);

	if(!_DN_gen_shader_storage_buffer(&vol->glChunkBufferID, sizeof(DNchunkGPU) * mapSize.x * mapSize.y * mapSize.x))
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_FATAL, "failed to generate chunk buffer");
		return NULL;
	}

	vol->voxelCap = DN_CHUNK_LENGTH * numChunks / 2;
	if(!_DN_gen_shader_storage_buffer(&vol->glVoxelBufferID, sizeof(DNvoxelGPU) * (vol->voxelCap + DN_CHUNK_LENGTH)))
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_FATAL, "failed to generate voxel buffer");
		return NULL;
	}

	//allocate CPU memory:
	//---------------------------------
	vol->map = DN_MALLOC(sizeof(DNchunkHandle) * mapSize.x * mapSize.y * mapSize.z);
	if(!vol->map)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_FATAL, "failed to allocate memory for map");
		return NULL;
	}

	//set all map tiles to empty:
	for(int i = 0; i < mapSize.x * mapSize.y * mapSize.z; i++)
		vol->map[i].flag = 0;

	vol->chunks = DN_MALLOC(sizeof(DNchunk) * numChunks);
	if(!vol->chunks)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_FATAL, "failed to allocate memory for chunks");
		return NULL;
	}

	//set all chunks to empty:
	for(int i = 0; i < numChunks; i++)
		_DN_clear_chunk(vol, i);

	vol->materials = DN_MALLOC(sizeof(DNmaterial) * DN_MAX_MATERIALS);
	if(!vol->materials)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_FATAL, "failed to allocate memory for materials");
		return NULL;
	}

	vol->lightingRequests = DN_MALLOC(sizeof(GLuint) * numChunks);
	if(!vol->lightingRequests)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_FATAL, "failed to allocate memory for lighting requests");
		return NULL;
	}

	vol->numVoxelNodes = vol->voxelCap / DN_CHUNK_LENGTH;
	vol->gpuVoxelLayout = DN_MALLOC(sizeof(DNvoxelNode) * (vol->voxelCap / 16));
	if(!vol->gpuVoxelLayout)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_FATAL, "failed to allocate memory for GPU voxel layout");
		return NULL;
	}

	//set up nodes (make them all max size and unloaded):
	for(int i = 0; i < vol->numVoxelNodes; i++)
	{
		vol->gpuVoxelLayout[i].chunkPos.x = -1;
		vol->gpuVoxelLayout[i].size = DN_CHUNK_LENGTH;
		vol->gpuVoxelLayout[i].startPos = i * DN_CHUNK_LENGTH;
	}

	//set data parameters:
	//---------------------------------
	vol->mapSize = mapSize;
	vol->chunkCap = numChunks;
	vol->nextChunk = 0;
	vol->numLightingRequests = 0;
	vol->lightingRequestCap = numChunks;

	//set default camera and lighting parameters:
	//---------------------------------
	vol->camPos = (DNvec3){0.0f, 0.0f, 0.0f};
	vol->camOrient = (DNvec3){0.0f, 0.0f, 0.0f};
	vol->camFOV = 90.0f;
	vol->camViewMode = 0;

	vol->sunDir = (DNvec3){1.0f, 1.0f, 1.0f};
	vol->sunStrength = (DNvec3){0.6f, 0.6f, 0.6f};
	vol->ambientLightStrength = (DNvec3){0.01f, 0.01f, 0.01f};
	vol->diffuseBounceLimit = 5;
	vol->specBounceLimit = 2;
	vol->shadowSoftness = 10.0f;

	vol->useCubemap = false;
	vol->skyGradientBot = (DNvec3){0.71f, 0.85f, 0.90f};
	vol->skyGradientTop = (DNvec3){0.00f, 0.45f, 0.74f};

	vol->frameNum = 0;
	vol->lastTime = 123.456f;

	return vol;
}

void DN_delete_volume(DNvolume* vol)
{
	glDeleteBuffers(1, &vol->glMapBufferID);
	glDeleteBuffers(1, &vol->glChunkBufferID);
	glDeleteBuffers(1, &vol->glVoxelBufferID);

	DN_FREE(vol->map);
	DN_FREE(vol->chunks);
	DN_FREE(vol->materials);
	DN_FREE(vol->lightingRequests);
	DN_FREE(vol->gpuVoxelLayout);
	DN_FREE(vol);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//FILE I/O:

//compresses a chunk to be stored on disk, returns the size, in bytes, of the compressed chunk
uint16_t _DN_compress_chunk(DNchunk chunk, DNvolume* vol, char* mem)
{
	char* orgMem = mem; //used to determine total size of compressed chunk
	_DN_write_buffer(&mem, &chunk.pos, sizeof(DNivec3));

	//if chunk is unused, dont write any more data:
	if(!DN_in_map_bounds(vol, chunk.pos))
		return sizeof(DNivec3);

	//determine if palette is needed + generate palette:
	int numNormal = 0;
	int numAlbedo = 0;
	DNbvec3 normalPalette[DN_CHUNK_LENGTH / 2]; //TODO: maybe implement a hashmap so that this is faster
	DNbvec3 albedoPalette[DN_CHUNK_LENGTH / 2];

	for(int z = 0; z < DN_CHUNK_SIZE; z++)
	for(int y = 0; y < DN_CHUNK_SIZE; y++)
	for(int x = 0; x < DN_CHUNK_SIZE; x++)
	{
		if(GET_MATERIAL_ID(chunk.voxels[x][y][z].normal) == DN_MATERIAL_EMPTY)
			continue;

		//get the albedo and normal for current voxel:
		uint32_t readNormal = chunk.voxels[x][y][z].normal;
		uint32_t readAlbedo = chunk.voxels[x][y][z].albedo;
		DNbvec3 normal = {(readNormal >> 16) & 0xFF, (readNormal >> 8 ) & 0xFF, readNormal & 0xFF};
		DNbvec3 albedo = {(readAlbedo >> 24) & 0xFF, (readAlbedo >> 16) & 0xFF, (readAlbedo >> 8) & 0xFF};

		//search for normal in palette, if palette is under size limit:
		if(numNormal < chunk.numVoxels / 2 && _DN_find_in_palette(normalPalette, numNormal, normal) < 0)
		{
			normalPalette[numNormal] = normal;
			numNormal++;
		}

		//search for albedo in palette, if palette is under size limit:
		if(numAlbedo < chunk.numVoxels	 / 2 && _DN_find_in_palette(albedoPalette, numAlbedo, albedo) < 0)
		{
			albedoPalette[numAlbedo] = albedo;
			numAlbedo++;
		}
	}

	//write normal palette data, or set palette size to 0 if it is too big:
	if(numNormal < chunk.numVoxels / 2)
	{
		uint8_t writeNumNormal = (uint8_t)numNormal;
		_DN_write_buffer(&mem, &writeNumNormal, sizeof(uint8_t));
		_DN_write_buffer(&mem, normalPalette, sizeof(DNbvec3) * numNormal);
	}
	else
	{
		numNormal = 0;
		uint8_t writeNumNormal = (uint8_t)numNormal;
		_DN_write_buffer(&mem, &writeNumNormal, sizeof(uint8_t));
	}

	//write albedo palette data, or set palette size to 0 if it is too big:
	if(numAlbedo < chunk.numVoxels / 2)
	{
		uint8_t writeNumAlbedo = (uint8_t)numAlbedo;
		_DN_write_buffer(&mem, &writeNumAlbedo, sizeof(uint8_t));
		_DN_write_buffer(&mem, albedoPalette, sizeof(DNbvec3) * numAlbedo);
	}
	else
	{
		numAlbedo = 0;
		uint8_t writeNumAlbedo = (uint8_t)numAlbedo;
		_DN_write_buffer(&mem, &writeNumAlbedo, sizeof(uint8_t));
	}

	//loop over each voxel and look to compress it:
	for(int i = 0; i < DN_CHUNK_LENGTH; i++)
	{
		DNivec3 pos = {i % DN_CHUNK_SIZE, (i / DN_CHUNK_SIZE) % DN_CHUNK_SIZE, i / (DN_CHUNK_SIZE * DN_CHUNK_SIZE)};
		uint8_t material = GET_MATERIAL_ID(chunk.voxels[pos.x][pos.y][pos.z].normal);
		_DN_write_buffer(&mem, &material, sizeof(uint8_t));

		char* numMem = mem; //where to write the number of voxels in the run length (determined later on)
		mem += sizeof(uint8_t);

		//loop over and determine run length of material:
		uint8_t num = 0;
		int j;
		for(j = i; j < DN_CHUNK_LENGTH; j++)
		{
			DNivec3 pos2 = {j % DN_CHUNK_SIZE, (j / DN_CHUNK_SIZE) % DN_CHUNK_SIZE, j / (DN_CHUNK_SIZE * DN_CHUNK_SIZE)};

			//if the voxels dont share a material, stop writing:
			if(num >= UINT8_MAX || GET_MATERIAL_ID(chunk.voxels[pos2.x][pos2.y][pos2.z].normal) != material)
				break;

			//increment number of voxels in material run:
			num++;
			if(material == DN_MATERIAL_EMPTY)
				continue;

			//write the normal, or its palette index if a palette is used:
			uint32_t readNormal = chunk.voxels[pos2.x][pos2.y][pos2.z].normal;
			DNbvec3 normal = {(readNormal >> 16) & 0xFF, (readNormal >> 8) & 0xFF, readNormal & 0xFF};
			if(numNormal > 0)
			{
				//search for palette index:
				uint8_t k = _DN_find_in_palette(normalPalette, numNormal, normal);					
				_DN_write_buffer(&mem, &k, sizeof(uint8_t));
			}
			else
				_DN_write_buffer(&mem, &normal, sizeof(DNbvec3));

			//write the albedo, or its palette index if a palette is used:
			uint32_t readAlbedo = chunk.voxels[pos2.x][pos2.y][pos2.z].albedo;
			DNbvec3 albedo = {(readAlbedo >> 24) & 0xFF, (readAlbedo >> 16) & 0xFF, (readAlbedo >> 8) & 0xFF};
			if(numAlbedo > 0)
			{
				//search for palette index:
				uint8_t k = _DN_find_in_palette(albedoPalette, numAlbedo, albedo);		
				_DN_write_buffer(&mem, &k, sizeof(uint8_t));
			}
			else
				_DN_write_buffer(&mem, &albedo, sizeof(DNbvec3));
		}

		//write number of voxels in current run:
		memcpy(numMem, &num, sizeof(uint8_t));
		i = j - 1;
	}

	//return total size of compressed chunk:
	return (uint16_t)(mem - orgMem);
}

//decompresses a chunk stored on disk
void _DN_decompress_chunk(char* mem, DNvolume* vol, DNchunk* chunk)
{
	//read position:
	_DN_read_buffer(&chunk->pos, &mem, sizeof(DNivec3));
	
	chunk->updated = false;
	chunk->numVoxels = 0;
	chunk->numVoxelsGpu = 0;

	//if chunk is unused, don't read any more data:
	if(!DN_in_map_bounds(vol, chunk->pos))
		return;

	//read palettes (if they are used):
	uint8_t numNormal = 0;
	uint8_t numAlbedo = 0;
	DNbvec3 normalPalette[256];
	DNbvec3 albedoPalette[256];

	_DN_read_buffer(&numNormal, &mem, sizeof(uint8_t));
	if(numNormal > 0)
		_DN_read_buffer(normalPalette, &mem, sizeof(DNbvec3) * numNormal);

	_DN_read_buffer(&numAlbedo, &mem, sizeof(uint8_t));
	if(numAlbedo > 0)
		_DN_read_buffer(albedoPalette, &mem, sizeof(DNbvec3) * numAlbedo);

	//read individual voxels:
	int numVoxelsRead = 0;
	while(numVoxelsRead < DN_CHUNK_LENGTH)
	{
		//read material:
		uint8_t material;
		_DN_read_buffer(&material, &mem, sizeof(uint8_t));

		uint8_t num; //number of voxels in material run
		_DN_read_buffer(&num, &mem, sizeof(uint8_t));

		//read voxels in run:
		for(int i = numVoxelsRead; i < numVoxelsRead + num; i++)
		{
			DNivec3 pos = {i % DN_CHUNK_SIZE, (i / DN_CHUNK_SIZE) % DN_CHUNK_SIZE, i / (DN_CHUNK_SIZE * DN_CHUNK_SIZE)};

			//if material is empty, simply set voxel and continue:
			if(material == DN_MATERIAL_EMPTY)
			{
				chunk->voxels[pos.x][pos.y][pos.z].normal = UINT32_MAX;
				continue;
			}

			//read normal, or its palette index if one is used:
			DNbvec3 normal;
			if(numNormal > 0)
			{
				uint8_t index;
				_DN_read_buffer(&index, &mem, sizeof(uint8_t));
				normal = normalPalette[index];
			}
			else
			{
				_DN_read_buffer(&normal, &mem, sizeof(DNbvec3));
			}

			//read albedo, or its palette index if one is used:
			DNbvec3 albedo;
			if(numAlbedo > 0)
			{
				uint8_t index;
				_DN_read_buffer(&index, &mem, sizeof(uint8_t));
				albedo = albedoPalette[index];
			}
			else
			{
				_DN_read_buffer(&albedo, &mem, sizeof(DNbvec3));
			}

			//set voxel:
			chunk->voxels[pos.x][pos.y][pos.z].normal = (material << 24) | (normal.x << 16) | (normal.y << 8) | normal.z;
			chunk->voxels[pos.x][pos.y][pos.z].albedo = (albedo.x << 24) | (albedo.y << 16) | (albedo.z << 8);
			chunk->numVoxels++;
		}

		//increase total voxels read by the number in the run
		numVoxelsRead += num;
	}
}

DNvolume* DN_load_volume(const char* filePath, unsigned int minChunks)
{
	//open file:
	//---------------------------------
	FILE* fptr = fopen(filePath, "rb");
	if(!fptr)
	{
		char message[256];
		sprintf(message, "failed to open file \"%s\" for reading", filePath);
		g_DN_message_callback(DN_MESSAGE_FILE_IO, DN_MESSAGE_ERROR, message);
		return NULL;
	}

	DNvolume* vol;

	//read map size:
	//---------------------------------
	DNuvec3 mapSize;
	fread(&mapSize, sizeof(DNuvec3), 1, fptr);
	vol = DN_create_volume(mapSize, minChunks);

	//read chunk cap and chunks:
	//---------------------------------
	size_t chunkCap;
	fread(&chunkCap, sizeof(size_t), 1, fptr);
	DN_set_max_chunks(vol, chunkCap);

	char* compressedMem = DN_MALLOC(sizeof(DNchunk) * 2); //allocate extra space in case compressed is larger
	for(int i = 0; i < chunkCap; i++)
	{
		uint16_t compressedSize;
		fread(&compressedSize, sizeof(uint16_t), 1, fptr);
		fread(compressedMem, compressedSize, 1, fptr);
		_DN_decompress_chunk(compressedMem, vol, &vol->chunks[i]);

		if(DN_in_map_bounds(vol, vol->chunks[i].pos))
		{
			unsigned int mapIndex = DN_FLATTEN_INDEX(vol->chunks[i].pos, mapSize);
			vol->map[mapIndex].flag = 1;
			vol->map[mapIndex].chunkIndex = i;
		}
	}
	DN_FREE(compressedMem);

	//read materials:
	//---------------------------------
	fread(vol->materials, sizeof(DNmaterial), DN_MAX_MATERIALS, fptr);

	//read camera parameters:
	//---------------------------------
	fread(&vol->camPos, sizeof(DNvec3), 1, fptr);
	fread(&vol->camOrient, sizeof(DNvec3), 1, fptr);
	fread(&vol->camFOV, sizeof(float), 1, fptr);
	fread(&vol->camViewMode, sizeof(uint32_t), 1, fptr);

	//read lighting parameters:
	//---------------------------------
	fread(&vol->sunDir, sizeof(DNvec3), 1, fptr);
	fread(&vol->sunStrength, sizeof(DNvec3), 1, fptr);
	fread(&vol->ambientLightStrength, sizeof(DNvec3), 1, fptr);
	fread(&vol->diffuseBounceLimit, sizeof(uint32_t), 1, fptr);
	fread(&vol->specBounceLimit, sizeof(uint32_t), 1, fptr);
	fread(&vol->shadowSoftness, sizeof(float), 1, fptr);

	//read sky parameters:
	//---------------------------------
	fread(&vol->skyGradientBot, sizeof(DNvec3), 1, fptr);
	fread(&vol->skyGradientTop, sizeof(DNvec3), 1, fptr);

	//close file and return:
	//---------------------------------
	fclose(fptr);
	return vol;
}

bool DN_save_volume(const char* filePath, DNvolume* vol)
{
	//open file:
	//---------------------------------
	FILE* fptr = fopen(filePath, "wb");
	if(!fptr)
	{
		char message[256];
		sprintf(message, "failed to open file \"%s\" for writing", filePath);
		g_DN_message_callback(DN_MESSAGE_FILE_IO, DN_MESSAGE_ERROR, message);
		return false;
	}

	//write map size and map:
	//---------------------------------
	fwrite(&vol->mapSize, sizeof(DNuvec3), 1, fptr);
	
	//write chunk cap and chunks:
	//---------------------------------
	fwrite(&vol->chunkCap, sizeof(size_t), 1, fptr);

	char* compressedBuffer = DN_MALLOC(sizeof(DNchunk) * 2); //allocate extra space in case compressed is larger
	for(int i = 0; i < vol->chunkCap; i++)
	{
		uint16_t compressedSize = _DN_compress_chunk(vol->chunks[i], vol, compressedBuffer);
		fwrite(&compressedSize, sizeof(uint16_t), 1, fptr);
		fwrite(compressedBuffer, compressedSize, 1, fptr);
	}
	DN_FREE(compressedBuffer);

	//write materials:
	//---------------------------------
	fwrite(vol->materials, sizeof(DNmaterial), DN_MAX_MATERIALS, fptr);

	//write camera parameters:
	//---------------------------------
	fwrite(&vol->camPos, sizeof(DNvec3), 1, fptr);
	fwrite(&vol->camOrient, sizeof(DNvec3), 1, fptr);
	fwrite(&vol->camFOV, sizeof(float), 1, fptr);
	fwrite(&vol->camViewMode, sizeof(uint32_t), 1, fptr);

	//write lighting parameters:
	//---------------------------------
	fwrite(&vol->sunDir, sizeof(DNvec3), 1, fptr);
	fwrite(&vol->sunStrength, sizeof(DNvec3), 1, fptr);
	fwrite(&vol->ambientLightStrength, sizeof(DNvec3), 1, fptr);
	fwrite(&vol->diffuseBounceLimit, sizeof(uint32_t), 1, fptr);
	fwrite(&vol->specBounceLimit, sizeof(uint32_t), 1, fptr);
	fwrite(&vol->shadowSoftness, sizeof(float), 1, fptr);

	//write sky parameters:
	//---------------------------------
	fwrite(&vol->skyGradientBot, sizeof(DNvec3), 1, fptr);
	fwrite(&vol->skyGradientTop, sizeof(DNvec3), 1, fptr);

	//close file and return
	//---------------------------------
	fclose(fptr);
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY:

int DN_add_chunk(DNvolume* vol, DNivec3 pos)
{
	//search for empty chunk:
	int i = vol->nextChunk;

	do
	{
		if(!DN_in_map_bounds(vol, vol->chunks[i].pos)) //if an empty chunk is found, use that one:
		{
			int mapIndex = DN_FLATTEN_INDEX(pos, vol->mapSize);
			vol->map[mapIndex].chunkIndex = i;
			vol->map[mapIndex].flag = 1;

			vol->chunks[i].pos = (DNivec3){pos.x, pos.y, pos.z};
			vol->nextChunk = (i == vol->chunkCap - 1) ? (0) : (i + 1);

			return i;
		}

		//jump to beginning if the end is reached:
		i++;
		if(i >= vol->chunkCap)
			i = 0;

	} while(i != vol->nextChunk);

	//if no empty chunk is found, increase capacity:
	size_t newCap = fmin(vol->chunkCap * 2, vol->mapSize.x * vol->mapSize.y * vol->mapSize.z);

	char message[256];
	sprintf(message, "automatically resizing chunk memory to accomodate %zi chunks (%zi bytes)", newCap, newCap * sizeof(DNchunk));
	g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_NOTE, message);

	i = vol->chunkCap;
	if(!DN_set_max_chunks(vol, newCap))
		return 0;

	//set chunk handle:
	int mapIndex = DN_FLATTEN_INDEX(pos, vol->mapSize);
	vol->map[mapIndex].chunkIndex = i;
	vol->map[mapIndex].flag = 1;

	//set chunk:
	vol->chunks[i].pos = (DNivec3){pos.x, pos.y, pos.z};
	vol->nextChunk = (i == vol->chunkCap - 1) ? (0) : (i + 1);

	return i;
}

void DN_remove_chunk(DNvolume* vol, DNivec3 pos)
{
	int mapIndex = DN_FLATTEN_INDEX(pos, vol->mapSize);
	vol->map[mapIndex].flag = 0;
	vol->nextChunk = vol->map[mapIndex].chunkIndex;
	_DN_clear_chunk(vol, vol->map[mapIndex].chunkIndex);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//UPDATING/DRAWING:

void DN_sync_gpu(DNvolume* vol, DNmemOp op, int lightingSplit)
{
	//whether voxel buffer need to be resized:
	bool resizeVoxels = false;

	//increase the frameNum (to determine which chunks should be updated when splitting lighting):
	vol->frameNum++;
	if(vol->frameNum >= lightingSplit)
		vol->frameNum = 0;

	//map the buffer:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glMapBufferID);
	DNchunkHandleGPU* gpuMap = (DNchunkHandleGPU*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
	DNchunkHandle* cpuMap = vol->map;

	//set lighting requests to 0:
	vol->numLightingRequests = 0;

	//loop through every map tile:
	for(int z = 0; z < vol->mapSize.z; z++)
	for(int y = 0; y < vol->mapSize.y; y++)
	for(int x = 0; x < vol->mapSize.x; x++)
	{
		//get position and index:
		DNivec3 pos = {x, y, z};
		int mapIndex = DN_FLATTEN_INDEX(pos, vol->mapSize);

		//get info for current chunk handle:
		DNchunkHandleGPU gpuHandle = gpuMap[mapIndex];
		int gpuFlag = gpuHandle.flags & 3;
		int gpuChunkIndex = gpuHandle.flags >> 4;
		bool gpuVisible = (gpuHandle.flags & 4) > 0;

		//increase the "time last used" flag:
		gpuMap[mapIndex].lastUsed++;

		//read data and add chunk to lighting request buffer:
		if(op != DN_WRITE)
			_DN_request_chunk_lighting(vol, cpuMap, mapIndex, gpuFlag, gpuChunkIndex, gpuVisible, lightingSplit);

		//write data and stream to gpu:
		if(op != DN_READ)
			_DN_stream_to_gpu(vol, cpuMap, gpuMap, pos, mapIndex, &gpuFlag, gpuChunkIndex, &resizeVoxels);
	}

	//sort the voxel layout to allow for adjacent nodes to be merged:
	const int MAX_SORT_COPIES = 10;
	_DN_sort_gpu_voxel_buffer(vol, gpuMap, MAX_SORT_COPIES);

	//unmap:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glMapBufferID);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	//resize voxel buffer if necessary:
	if(resizeVoxels)
	{
		size_t newCap = fmin(vol->voxelCap * 2, vol->chunkCap * DN_CHUNK_LENGTH);

		char message[256];
		sprintf(message, "automatically resizing voxel buffer to accomodate %zi GPU voxels (%zi bytes)", newCap, newCap * sizeof(DNvoxelGPU));
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_NOTE, message);

		DN_set_max_voxels_gpu(vol, newCap);
	}

	//memory barrier to avoid any strange mem issues:
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

void DN_set_view_projection_matrices(DNvolume* vol, float aspectRatio, float nearPlane, float farPlane, DNmat4* view, DNmat4* projection)
{
	DNmat3 rotate = DN_mat4_top_left(DN_mat4_rotate_euler(vol->camOrient));
	DNvec3 camFront;
	DNvec3 camPlaneU;
	DNvec3 camPlaneV;

	if(aspectRatio < 1.0f)
	{
		camFront  = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, aspectRatio / tanf(DN_deg_to_rad(vol->camFOV * 0.5f)) });
		camPlaneU = DN_mat3_mult_vec3(rotate, (DNvec3){-1.0f, 0.0f, 0.0f});
		camPlaneV = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 1.0f * aspectRatio, 0.0f});
	}
	else
	{
		camFront  = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, 1.0f / tanf(DN_deg_to_rad(vol->camFOV * 0.5f)) });
		camPlaneU = DN_mat3_mult_vec3(rotate, (DNvec3){-1.0f / aspectRatio, 0.0f, 0.0f});
		camPlaneV = DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 1.0f, 0.0f});
	}
	
	*view = DN_mat4_lookat(vol->camPos, DN_vec3_add(vol->camPos, camFront), (DNvec3){0.0f, 1.0f, 0.0f});
	*projection = DN_mat4_perspective(vol->camFOV, 1.0f / aspectRatio, nearPlane, farPlane);
}

void DN_draw(DNvolume* vol, GLuint outputTexture, DNmat4 view, DNmat4 projection, int rasterColorTexture, int rasterDepthTexture)
{
	glUseProgram(g_drawProgram);

	//bind buffers:
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vol->glMapBufferID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vol->glChunkBufferID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, vol->glVoxelBufferID);
	glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//send over material data:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_materialBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DNmaterial) * DN_MAX_MATERIALS, vol->materials);

	//find width and height of output texture:
	unsigned int w, h;
	glBindTexture(GL_TEXTURE_2D, outputTexture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	//send rasterization pipeline textures if needed:
	DN_program_uniform_uint(g_drawProgram, "composeRasterized", rasterColorTexture >= 0 && rasterDepthTexture >= 0);
	DN_program_uniform_int(g_drawProgram, "colorSample", 0);
	DN_program_uniform_int(g_drawProgram, "depthSample", 1);
	if(rasterColorTexture >= 0 && rasterDepthTexture >= 0)
	{
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, rasterColorTexture);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, rasterDepthTexture);
	}

	//to calculate ray direction, we want to rotate screen space positions according to the view matrix, but not translate them
	DNmat4 centeredView = view;
	centeredView.m[3][0] = 0.0;
	centeredView.m[3][1] = 0.0;
	centeredView.m[3][2] = 0.0;

	//precalculate inverse matrices:
	DNmat4 invView = DN_mat4_inv(view);
	DNmat4 invCenteredView = DN_mat4_inv(centeredView);
	DNmat4 invProjection = DN_mat4_inv(projection);

	//send sky data:
	DN_program_uniform_uint(g_drawProgram, "useCubemap", vol->useCubemap);
	DN_program_uniform_int(g_drawProgram, "skyCubemap", 2);
	if(vol->useCubemap)
	{
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, vol->glCubemapTex);
	}
	else
	{
		DN_program_uniform_vec3(g_drawProgram, "skyGradientBot", &vol->skyGradientBot);
		DN_program_uniform_vec3(g_drawProgram, "skyGradientTop", &vol->skyGradientTop);
	}

	//send lighting and camera positioning info:
	DN_program_uniform_vec3(g_drawProgram, "sunStrength", &vol->sunStrength);
	DN_program_uniform_uint(g_drawProgram, "viewMode", vol->camViewMode);
	DN_program_uniform_vec3(g_drawProgram, "ambientStrength", &vol->ambientLightStrength);
	DN_program_uniform_mat4(g_drawProgram, "invViewMat", &invView);
	DN_program_uniform_mat4(g_drawProgram, "invCenteredViewMat", &invCenteredView);
	DN_program_uniform_mat4(g_drawProgram, "invProjectionMat", &invProjection);
	glUniform3uiv(glGetUniformLocation(g_drawProgram, "mapSize"), 1, (GLuint*)&vol->mapSize);

	//dispatch and mem barrier:
	glDispatchCompute(w / DRAW_WORKGROUP_SIZE, h / DRAW_WORKGROUP_SIZE, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void DN_update_lighting(DNvolume* vol, int numDiffuseSamples, int maxDiffuseSamples, float time)
{
	//to ensure that each chunk gets the same seed for random sampling:
	if(vol->frameNum == 0)
		vol->lastTime = time;
		
	glUseProgram(g_lightingProgram);

	//bind buffers:
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vol->glChunkBufferID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vol->glMapBufferID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, vol->glVoxelBufferID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_lightingRequestBuffer);

	//resize lighting request buffer if needed:
	if(vol->numLightingRequests > g_maxLightingRequests)
	{
		size_t newCap = g_maxLightingRequests;
		while(newCap < vol->numLightingRequests )
			newCap *= 2;

		char message[256];
		sprintf(message, "automatically resizing lighting request buffer to accomodate %zi requests (%zi bytes)", newCap, newCap * sizeof(GLuint));
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_NOTE, message);

		_DN_clear_gl_errors();
		glBufferData(GL_SHADER_STORAGE_BUFFER, newCap * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		if(_DN_gl_error())
		{
			g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_ERROR, "failed to resize lighting request buffer");
			return;
		}
		g_maxLightingRequests = newCap;
	}

	//send lighting requests:
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, vol->numLightingRequests  * sizeof(GLuint), vol->lightingRequests);

	//send sky data:
	DN_program_uniform_uint(g_lightingProgram, "useCubemap", vol->useCubemap);
	DN_program_uniform_int(g_lightingProgram, "skyCubemap", 2);
	if(vol->useCubemap)
	{
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, vol->glCubemapTex);
	}
	else
	{
		DN_program_uniform_vec3(g_lightingProgram, "skyGradientBot", &vol->skyGradientBot);
		DN_program_uniform_vec3(g_lightingProgram, "skyGradientTop", &vol->skyGradientTop);
	}

	//send lighting and cam data:
	DN_program_uniform_vec3(g_lightingProgram, "camPos", &vol->camPos);
	DN_program_uniform_float(g_lightingProgram, "time", vol->lastTime);
	DN_program_uniform_uint(g_lightingProgram, "numDiffuseSamples", numDiffuseSamples);
	DN_program_uniform_uint(g_lightingProgram, "maxDiffuseSamples", maxDiffuseSamples);
	DN_program_uniform_uint(g_lightingProgram, "diffuseBounceLimit", vol->diffuseBounceLimit);
	DN_program_uniform_uint(g_lightingProgram, "specularBounceLimit", vol->specBounceLimit);
	DNvec3 normalizedSunDir = DN_vec3_normalize(vol->sunDir);
	DN_program_uniform_vec3(g_lightingProgram, "sunDir", &normalizedSunDir);
	DN_program_uniform_vec3(g_lightingProgram, "sunStrength", &vol->sunStrength);
	DN_program_uniform_float(g_lightingProgram, "shadowSoftness", vol->shadowSoftness);
	DN_program_uniform_vec3(g_lightingProgram, "ambientStrength", &vol->ambientLightStrength);
	glUniform3uiv(glGetUniformLocation(g_lightingProgram, "mapSize"), 1, (GLuint*)&vol->mapSize);

	//dispatch and mem barrier:
	glDispatchCompute(vol->numLightingRequests, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP SETTINGS:

bool DN_set_map_size(DNvolume* vol, DNuvec3 size)
{
	//allocate new map:
	DNchunkHandle* newMap = DN_MALLOC(sizeof(DNchunkHandle) * size.x * size.y * size.z);
	if(!newMap)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate memory for map");
		return false;
	}

	//loop over and copy map data:
	for(int z = 0; z < size.z; z++)
	for(int y = 0; y < size.y; y++)
	for(int x = 0; x < size.x; x++)
	{
		DNivec3 pos = {x, y, z};

		int oldIndex = DN_FLATTEN_INDEX(pos, vol->mapSize);
		int newIndex = DN_FLATTEN_INDEX(pos, size);

		newMap[newIndex] = DN_in_map_bounds(vol, pos) ? vol->map[oldIndex] : (DNchunkHandle){0};
	}

	DN_FREE(vol->map);
	vol->map = newMap;
	vol->mapSize = size;

	//remove chunks that are no longer indexed:
	for(int i = 0; i < vol->chunkCap; i++)
		if(!DN_in_map_bounds(vol, vol->chunks[i].pos))
			_DN_clear_chunk(vol, i);

	//allocate new gpu buffer:
	_DN_clear_gl_errors();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glMapBufferID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DNchunkHandleGPU) * size.x * size.y * size.z, vol->map, GL_DYNAMIC_DRAW);
	if(_DN_gl_error())
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate map buffer");
		return false;
	}

	//allocate new gpu chunk buffer:
	_DN_clear_gl_errors();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glChunkBufferID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(DNchunkGPU) * size.x * size.y * size.z, vol->map, GL_DYNAMIC_DRAW);
	if(_DN_gl_error())
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate chunk buffer");
		return false;
	}

	return true;
}

bool DN_set_max_chunks(DNvolume* vol, size_t num)
{
	//allocate space:
	DNchunk* newChunks = DN_REALLOC(vol->chunks, sizeof(DNchunk) * num);
	if(!newChunks)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate memory for chunks");
		return false;
	}
	vol->chunks = newChunks;

	//clear new chunks
	for(int i = vol->chunkCap; i < num; i++)
		_DN_clear_chunk(vol, i);

	vol->chunkCap = num;
	return true;
}

bool DN_set_max_voxels_gpu(DNvolume* vol, size_t num)
{
	//resize chunk layout memory:
	DNvoxelNode* newGpuVoxelLayout = DN_REALLOC(vol->gpuVoxelLayout, sizeof(DNvoxelNode) * (num / 16));
	if(!newGpuVoxelLayout)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate memory for GPU voxel layput");
		return false;
	}
	vol->gpuVoxelLayout = newGpuVoxelLayout;

	//resize buffer:
	_DN_clear_gl_errors();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glVoxelBufferID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (num + DN_CHUNK_LENGTH) * sizeof(DNvoxelGPU), NULL, GL_DYNAMIC_DRAW);
	if(_DN_gl_error())
	{
		g_DN_message_callback(DN_MESSAGE_GPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate voxel buffer");
		return false;
	}

	//set voxel cap nad num nodes:
	vol->numVoxelNodes = num / DN_CHUNK_LENGTH;
	vol->voxelCap = num;

	//clear voxel layout memory:
	for(size_t i = 0; i < vol->numVoxelNodes; i++)
	{
		vol->gpuVoxelLayout[i].chunkPos.x = -1;
		vol->gpuVoxelLayout[i].size = DN_CHUNK_LENGTH;
		vol->gpuVoxelLayout[i].startPos = i * DN_CHUNK_LENGTH;
	}

	//clear gpu map:
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glMapBufferID);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, NULL);
	vol->numLightingRequests = 0;

	return true;
}

bool DN_set_max_lighting_requests(DNvolume* vol, size_t num)
{
	GLuint* newRequests = DN_REALLOC(vol->lightingRequests, sizeof(GLuint) * num);
	if(!newRequests)
	{
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_ERROR, "failed to reallocate memory for lighting requests");
		return false;
	}

	vol->lightingRequests = newRequests;
	vol->lightingRequestCap = num;
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP UTILITY:

bool DN_in_map_bounds(DNvolume* vol, DNivec3 pos)
{
	return pos.x < vol->mapSize.x && pos.y < vol->mapSize.y && pos.z < vol->mapSize.z && pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

bool DN_in_chunk_bounds(DNivec3 pos)
{
	return pos.x < DN_CHUNK_SIZE && pos.y < DN_CHUNK_SIZE && pos.z < DN_CHUNK_SIZE && pos.x >= 0 && pos.y >= 0 && pos.z >= 0;
}

DNvoxel DN_get_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos)
{
	return DN_decompress_voxel(DN_get_compressed_voxel(vol, mapPos, chunkPos));
}

DNcompressedVoxel DN_get_compressed_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos)
{
	return vol->chunks[vol->map[DN_FLATTEN_INDEX(mapPos, vol->mapSize)].chunkIndex].voxels[chunkPos.x][chunkPos.y][chunkPos.z];
}

void DN_set_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos, DNvoxel voxel)
{
	DN_set_compressed_voxel(vol, mapPos, chunkPos, DN_compress_voxel(voxel));
}

void DN_set_compressed_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos, DNcompressedVoxel voxel)
{
	//add new chunk if the requested chunk doesn't yet exist:
	int mapIndex = DN_FLATTEN_INDEX(mapPos, vol->mapSize);
	if(vol->map[mapIndex].flag == 0)
	{
		if(GET_MATERIAL_ID(voxel.normal) == DN_MATERIAL_EMPTY) //if adding an empty voxel to an empty chunk, just return
			return;

		DN_add_chunk(vol, mapPos);
	}
	
	int chunkIndex = vol->map[mapIndex].chunkIndex;

	//change number of voxels in map:
	int oldMat = GET_MATERIAL_ID(vol->chunks[chunkIndex].voxels[chunkPos.x][chunkPos.y][chunkPos.z].normal);
	int newMat = GET_MATERIAL_ID(voxel.normal);

	if(oldMat == DN_MATERIAL_EMPTY && newMat != DN_MATERIAL_EMPTY) //if old voxel was empty and new one is not, increment the number of voxels
		vol->chunks[chunkIndex].numVoxels++;
	else if(oldMat != DN_MATERIAL_EMPTY && newMat == DN_MATERIAL_EMPTY) //if old voxel was not empty and new one is, decrement the number of voxels
	{
		vol->chunks[chunkIndex].numVoxels--;

		//check if the chunk should be removed:
		if(vol->chunks[chunkIndex].numVoxels <= 0)
		{
			DN_remove_chunk(vol, mapPos);
			return;
		}
	}

	//actually set new voxel:
	vol->chunks[chunkIndex].voxels[chunkPos.x][chunkPos.y][chunkPos.z] = voxel;
	vol->chunks[chunkIndex].updated = 1;
}

void DN_remove_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos)
{
	//change number of voxels in map (only if old voxel was solid)
	int chunkIndex = vol->map[DN_FLATTEN_INDEX(mapPos, vol->mapSize)].chunkIndex;
	if(DN_does_voxel_exist(vol, mapPos, chunkPos))
	{
		vol->chunks[chunkIndex].numVoxels--;

		//remove chunk if no more voxels exist:
		if(vol->chunks[chunkIndex].numVoxels <= 0)
		{
			DN_remove_chunk(vol, mapPos);
			return;
		}
	}

	//clear voxel:
	vol->chunks[chunkIndex].voxels[chunkPos.x][chunkPos.y][chunkPos.z].normal = UINT32_MAX; 
	vol->chunks[chunkIndex].updated = 1;
}

bool DN_does_chunk_exist(DNvolume* vol, DNivec3 pos)
{
	return vol->map[DN_FLATTEN_INDEX(pos, vol->mapSize)].flag >= 1;
}

bool DN_does_voxel_exist(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos)
{
	int material = GET_MATERIAL_ID(DN_get_compressed_voxel(vol, mapPos, chunkPos).normal);
	return material != DN_MATERIAL_EMPTY;
}

#define sign(n) ((n) > 0) ? 1 : (((n) < 0) ? -1 : 0)
bool DN_step_map(DNvolume* vol, DNvec3 rayDir, DNvec3 rayPos, int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal)
{
	*hitNormal = (DNivec3){-1000, -1000, -1000};

	//scale to use voxel-level coordinates
	rayPos = DN_vec3_scale(rayPos, DN_CHUNK_SIZE);

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

	int numSteps = 0;
	while(numSteps < maxSteps)
	{
		//check if in bounds:
		DNivec3 mapPos;
		DNivec3 chunkPos;
		DN_separate_position(pos, &mapPos, &chunkPos);

		if(pos.x < 0)
			mapPos.x--;
		if(pos.y < 0)
			mapPos.y--;
		if(pos.z < 0)
			mapPos.z--;

		//check if voxel exists:
		if(DN_in_map_bounds(vol, mapPos) && DN_does_chunk_exist(vol, mapPos) && DN_does_voxel_exist(vol, mapPos, chunkPos))
		{
			*hitVoxel = DN_get_voxel(vol, mapPos, chunkPos);
			*hitPos = pos;
			return true;
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
}

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

void DN_separate_position(DNivec3 pos, DNivec3* mapPos, DNivec3* chunkPos)
{
	*mapPos   = (DNivec3){pos.x / DN_CHUNK_SIZE, pos.y / DN_CHUNK_SIZE, pos.z / DN_CHUNK_SIZE};
	*chunkPos = (DNivec3){pos.x % DN_CHUNK_SIZE, pos.y % DN_CHUNK_SIZE, pos.z % DN_CHUNK_SIZE};
}

DNvec3 DN_cam_dir(DNvec3 orient)
{
	DNmat3 rotate = DN_mat4_top_left(DN_mat4_rotate_euler(orient));
	return DN_mat3_mult_vec3(rotate, (DNvec3){ 0.0f, 0.0f, 1.0f });
}

DNcompressedVoxel DN_compress_voxel(DNvoxel voxel)
{
	DNcompressedVoxel res;

	voxel.normal = DN_vec3_min(voxel.normal, (DNvec3){ 1.0f,  1.0f,  1.0f});
	voxel.normal = DN_vec3_max(voxel.normal, (DNvec3){-1.0f, -1.0f, -1.0f});
	DNuvec3 normal = {((int)(voxel.normal.x * 255.0f) + 255) / 2, ((int)(voxel.normal.y * 255.0f) + 255) / 2, ((int)(voxel.normal.z * 255.0f) + 255) / 2};

	res.normal = (voxel.material << 24) | (normal.x       << 16) | (normal.y       << 8) | (normal.z);
	res.albedo = (voxel.albedo.r << 24) | (voxel.albedo.g << 16) | (voxel.albedo.b << 8);

	return res;
}

DNvoxel DN_decompress_voxel(DNcompressedVoxel voxel)
{
	DNvoxel res;

	const float inv255 = 1.0f / 255.0f;
	DNivec3 normalRead = (DNivec3){(voxel.normal >> 16) & 0xFF, (voxel.normal >> 8) & 0xFF, voxel.normal & 0xFF};
	normalRead = (DNivec3){normalRead.x * 2 - 255, normalRead.y * 2 - 255, normalRead.z * 2 - 255};

	res.normal = DN_vec3_scale((DNvec3){normalRead.x, normalRead.y, normalRead.z}, inv255);
	res.material = voxel.normal >> 24;
	res.albedo = (DNcolor){(voxel.albedo >> 24) & 0xFF, (voxel.albedo >> 16) & 0xFF, (voxel.albedo >> 8) & 0xFF};

	return res;
}

//--------------------------------------------------------------------------------------------------------------------------------//
//HELPER FUNCTIONS:

//gl error checking:

static void _DN_clear_gl_errors()
{
	while(glGetError() != GL_NO_ERROR);
}

static bool _DN_gl_error()
{
	return glGetError() != GL_NO_ERROR;
}

//initialization:

static bool _DN_gen_shader_storage_buffer(GLuint* dest, size_t size)
{
	_DN_clear_gl_errors();
	unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer); //bind
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW); //allocate
	if(_DN_gl_error())
	{	
		glDeleteBuffers(1, &buffer);
		return false;
	}

	*dest = buffer;

	return true;
}

static void _DN_clear_chunk(DNvolume* vol, int index)
{
	vol->chunks[index].pos = (DNivec3){-1, -1, -1};
	vol->chunks[index].updated = false;
	vol->chunks[index].numVoxels = 0;

	for(int z = 0; z < DN_CHUNK_SIZE; z++)
	for(int y = 0; y < DN_CHUNK_SIZE; y++)
	for(int x = 0; x < DN_CHUNK_SIZE; x++)
		vol->chunks[index].voxels[x][y][z].normal = UINT32_MAX;
}

//file i/o and compression:

static void _DN_write_buffer(char** dest, void* src, size_t size)
{
	memcpy(*dest, src, size);
	*dest += size;
}

static void _DN_read_buffer(void* dest, char** src, size_t size)
{
	memcpy(dest, *src, size);
	*src += size;
}

static int _DN_find_in_palette(DNbvec3* palette, int paletteSize, DNbvec3 item)
{
	for(int i = 0; i < paletteSize; i++)
		if(palette[i].x == item.x && palette[i].y == item.y && palette[i].z == item.z)
			return i;

	return -1;
}

//cpu/gpu streaming:

//returns whether a voxel's face is visible
static bool _DN_check_face_visible(DNvolume* vol, DNchunk chunk, DNivec3 pos)
{
	return !DN_in_chunk_bounds(pos) || GET_MATERIAL_ID(chunk.voxels[pos.x][pos.y][pos.z].normal) == DN_MATERIAL_EMPTY || vol->materials[GET_MATERIAL_ID(chunk.voxels[pos.x][pos.y][pos.z].normal)].opacity < 1.0f;
}

//converts a DNchunk to a DNchunkGPU
static DNchunkGPU _DN_chunk_to_gpu(DNvolume* vol, DNchunk chunk, int* numVoxels, DNvoxelGPU* voxels)
{
	DNchunkGPU res;
	res.pos = chunk.pos;
	res.numLightingSamples = 0;

	//reset bitmask:
	for(int i = 0; i < 16; i++)
		res.bitMask[i] = 0;

	int n = 0;
	for(int z = 0; z < DN_CHUNK_SIZE; z++)
	for(int y = 0; y < DN_CHUNK_SIZE; y++)
	for(int x = 0; x < DN_CHUNK_SIZE; x++)
	{
		//get index:
		DNivec3 pos = (DNivec3){x, y, z};
		unsigned int index = pos.x + DN_CHUNK_SIZE * (pos.y + DN_CHUNK_SIZE * pos.z);

		//set partial count:
		if((index & 31) == 0 && index != 0 && ((index >> 5) & 3) == 0)
			res.partialCounts[(index >> 7) - 1] = n;

		//exit if voxel is empty or if not visible:
		if(GET_MATERIAL_ID(chunk.voxels[x][y][z].normal) == DN_MATERIAL_EMPTY)
			continue;

		bool visible = false;
		visible = visible || _DN_check_face_visible(vol, chunk, (DNivec3){x + 1, y, z});
		visible = visible || _DN_check_face_visible(vol, chunk, (DNivec3){x - 1, y, z});
		visible = visible || _DN_check_face_visible(vol, chunk, (DNivec3){x, y + 1, z});
		visible = visible || _DN_check_face_visible(vol, chunk, (DNivec3){x, y - 1, z});
		visible = visible || _DN_check_face_visible(vol, chunk, (DNivec3){x, y, z + 1});
		visible = visible || _DN_check_face_visible(vol, chunk, (DNivec3){x, y, z - 1});
		if(!visible)
			continue;

		//set bitmask:
		res.bitMask[index >> 5] |= 1 << (index & 31);

		//linearize albedo:
		uint32_t readAlbedo = chunk.voxels[x][y][z].albedo;
		DNcolor albedo = {(readAlbedo >> 24) & 0xFF, (readAlbedo >> 16) & 0xFF, (readAlbedo >> 8) & 0xFF};

		DNvec3 linearized = DN_vec3_scale((DNvec3){albedo.r, albedo.g, albedo.b}, 0.00392156862f);
		linearized.x = powf(linearized.x, DN_GAMMA);
		linearized.y = powf(linearized.y, DN_GAMMA);
		linearized.z = powf(linearized.z, DN_GAMMA);
		linearized = DN_vec3_scale(linearized, 255.0f);

		albedo = (DNcolor){linearized.x, linearized.y, linearized.z};
		readAlbedo = (albedo.r << 24) | (albedo.g << 16) | (albedo.b << 8);

		//set voxel:
		DNvoxelGPU vox;
		vox.normal = chunk.voxels[x][y][z].normal;
		vox.directLight = readAlbedo;
		vox.diffuseLight = 0;
		vox.specLight = 0;
		voxels[n++] = vox;
	}

	*numVoxels = n;
	return res;
}

static void _DN_request_chunk_lighting(DNvolume* vol, DNchunkHandle* cpuMap, int mapIndex, int gpuFlag, int gpuChunkIndex, bool gpuVisible, int lightingSplit)
{
	//if chunk isnt loaded or visible, return:
	if(gpuFlag != 2 || !gpuVisible)
		return;

	//if chunk isnt included in current lighting split and isnt updated, return:
	if(mapIndex % lightingSplit != vol->frameNum && !vol->chunks[cpuMap[mapIndex].chunkIndex].updated)
		return;

	//resize the lighting request buffer if not large enough:
	if(vol->numLightingRequests + (DN_CHUNK_LENGTH / LIGHTING_WORKGROUP_SIZE) >= vol->lightingRequestCap)
	{
		size_t newCap = vol->lightingRequestCap * 2;

		char message[256];
		sprintf(message, "automatically resizing lighting request memory to accomodate %zi requests (%zi bytes)", newCap, newCap * sizeof(GLuint));
		g_DN_message_callback(DN_MESSAGE_CPU_MEMORY, DN_MESSAGE_NOTE, message);

		if(!DN_set_max_lighting_requests(vol, newCap))
			return;
	}

	//add requests (enough to cover all the voxels)
	for(int i = 0; i < vol->chunks[cpuMap[mapIndex].chunkIndex].numVoxelsGpu; i += LIGHTING_WORKGROUP_SIZE)
		vol->lightingRequests[vol->numLightingRequests++] = (mapIndex << 4) | (i / LIGHTING_WORKGROUP_SIZE);;
}

static void _DN_stream_to_gpu(DNvolume* vol, DNchunkHandle* cpuMap, DNchunkHandleGPU* gpuMap, DNivec3 pos, int mapIndex, int* gpuFlag, int gpuChunkIndex, bool* resizeVoxels)
{
	//if a chunk was added to the cpu map, add it to the gpu map:
	if(cpuMap[mapIndex].flag != 0 && *gpuFlag == 0)
	{
		gpuMap[mapIndex].flags = 1;
		*gpuFlag = 1;
	}
	else if(cpuMap[mapIndex].flag == 0 && *gpuFlag != 0) //if chunk was removed from the cpu map, remove it from the gpu map
	{
		if(*gpuFlag == 2)
			_DN_unload_voxels(vol, mapIndex, gpuChunkIndex);

		gpuMap[mapIndex].flags = 0;
		*gpuFlag = 0;
	}

	//if updated, unload and request it to let the streaming system handle it
	if(*gpuFlag == 2 && vol->chunks[cpuMap[mapIndex].chunkIndex].updated)
	{
		_DN_unload_voxels(vol, mapIndex, gpuChunkIndex);

		gpuMap[mapIndex].flags = 3;
		*gpuFlag = 3;
	}

	//if flag = 3 (requested), try to load a new chunk:
	if(*gpuFlag == 3 && cpuMap[mapIndex].flag != 0)
	{
		unsigned int numVoxels;
		DNvoxelGPU gpuVoxels[DN_CHUNK_LENGTH];
		DNchunkGPU gpuChunk = _DN_chunk_to_gpu(vol, vol->chunks[cpuMap[mapIndex].chunkIndex], &numVoxels, gpuVoxels);
		vol->chunks[cpuMap[mapIndex].chunkIndex].numVoxelsGpu = numVoxels;

		gpuMap[mapIndex].flags = 2;
		gpuMap[mapIndex].lastUsed = 0;

		_DN_stream_chunk(vol, mapIndex, gpuChunk);
		if(_DN_stream_voxels(vol, pos, gpuMap, mapIndex, numVoxels, gpuVoxels))
			*resizeVoxels = true;
	}

	//set updated flag to false:
	if(cpuMap[mapIndex].flag != 0)
		vol->chunks[cpuMap[mapIndex].chunkIndex].updated = false;
}

static void _DN_unload_voxels(DNvolume* vol, int mapIndex, int chunkIndex)
{
	for(int i = 0; i < vol->numVoxelNodes; i++)
		if(DN_in_map_bounds(vol, vol->gpuVoxelLayout[i].chunkPos) && DN_FLATTEN_INDEX(vol->gpuVoxelLayout[i].chunkPos, vol->mapSize) == mapIndex)
		{
			vol->gpuVoxelLayout[i].chunkPos.x = -1;
			break;
		}
}

static void _DN_stream_chunk(DNvolume* vol, int mapIndex, DNchunkGPU chunk)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glChunkBufferID);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, mapIndex * sizeof(DNchunkGPU), sizeof(DNchunkGPU), &chunk);
}

static bool _DN_stream_voxels(DNvolume* vol, DNivec3 pos, DNchunkHandleGPU* mapGPU, int mapIndex, int numVoxels, DNvoxelGPU* voxels)
{
	//calculate needed node size:
	int nodeSize = 16;
	while(nodeSize < numVoxels)
		nodeSize *= 2;

	//loop over and find the smallest and least used node:
	uint32_t maxTime = 0;
	int maxTimeNodeSize = -1;
	int maxTimeIndex = -1;
	int maxTimeMapIndex = -1;

	for(int i = 0; i < vol->numVoxelNodes; i++)
	{
		//continue if node is too small:
		if(vol->gpuVoxelLayout[i].size < nodeSize)
			continue;

		//get position and index of chunk:
		DNivec3 currentPos = vol->gpuVoxelLayout[i].chunkPos;
		unsigned int currentIndex = DN_FLATTEN_INDEX(currentPos, vol->mapSize);

		//see if chunk is older or has a smaller node:
		uint32_t time = DN_in_map_bounds(vol, currentPos) ? mapGPU[currentIndex].lastUsed : UINT32_MAX;
		if(time > maxTime || (time == UINT32_MAX && vol->gpuVoxelLayout[i].size < maxTimeNodeSize))
		{
			maxTime = time;
			maxTimeNodeSize = vol->gpuVoxelLayout[i].size;
			maxTimeIndex = i;
			maxTimeMapIndex = currentIndex;
		}

		//if unloaded chunk with optimal node size is found, early exit:
		if(maxTimeNodeSize == nodeSize && maxTime == UINT32_MAX)
			break;
	}

	//if there isn't enough space, unload corresponding chunk and return true to double the size of the voxel buffer:
	if(maxTimeIndex < 0 || maxTime <= 1)
	{
		mapGPU[mapIndex].flags = 1;
		return true;
	}

	//unload old one if overwritten:
	if(DN_in_map_bounds(vol, vol->gpuVoxelLayout[maxTimeIndex].chunkPos))
		mapGPU[maxTimeMapIndex].flags = 1;

	//split a node if needed
	if(maxTimeNodeSize > nodeSize)
	{
		//determine how many splits need to be made:
		int numAdded = 0;
		while(maxTimeNodeSize > nodeSize)
		{
			maxTimeNodeSize /= 2;
			numAdded++;
		}

		//shift all mem over:
		int orgStartPos = vol->gpuVoxelLayout[maxTimeIndex].startPos;
		memmove(&vol->gpuVoxelLayout[maxTimeIndex + numAdded], &vol->gpuVoxelLayout[maxTimeIndex], sizeof(DNvoxelNode) * (vol->numVoxelNodes - maxTimeIndex));

		//set new nodes:
		int mult = 1;
		for(int i = maxTimeIndex; i <= maxTimeIndex + numAdded; i++)
		{
			vol->gpuVoxelLayout[i].size = nodeSize * mult;
			vol->gpuVoxelLayout[i].startPos = orgStartPos + (i == maxTimeIndex ? 0 : nodeSize * mult);
			vol->gpuVoxelLayout[i].chunkPos.x = -1;

			if(i > maxTimeIndex)
				mult *= 2;
		}

		vol->numVoxelNodes += numAdded;
	}

	//send data:
	vol->gpuVoxelLayout[maxTimeIndex].chunkPos = pos;
	mapGPU[mapIndex].voxelIndex = vol->gpuVoxelLayout[maxTimeIndex].startPos;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glVoxelBufferID);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, vol->gpuVoxelLayout[maxTimeIndex].startPos * sizeof(DNvoxelGPU), numVoxels * sizeof(DNvoxelGPU), voxels);

	return false;
}

static void _DN_sort_gpu_voxel_buffer(DNvolume* vol, DNchunkHandleGPU* gpuMap, int maxCopies)
{
	//desired layout: all used nodes (in arbitrary order) | all unused nodes (in descending order)

	int numCopies = 0;
	for(int i = 0; i < vol->numVoxelNodes - 1; i++) //TODO: there's definitely a better algo than bubble sort, but for now its fine
	{
		//sort:
		DNvoxelNode cur  = vol->gpuVoxelLayout[i];
		DNvoxelNode next = vol->gpuVoxelLayout[i + 1];

		//if the current one is in use, dont swap
		if(DN_in_map_bounds(vol, cur.chunkPos))
			continue;

		//if the next one is in use (the current one isnt) or is smaller than the current one, swap them
		if(DN_in_map_bounds(vol, next.chunkPos) || next.size < cur.size)
		{	
			numCopies++;

			//copy to the empty space at the end of the buffer, then swap
			if(DN_in_map_bounds(vol, next.chunkPos))
			{
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, vol->glVoxelBufferID);
				glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER, next.startPos * sizeof(DNvoxelGPU), vol->voxelCap * sizeof(DNvoxelGPU), next.size * sizeof(DNvoxelGPU));
				glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER, vol->voxelCap * sizeof(DNvoxelGPU), (next.startPos - cur.size) * sizeof(DNvoxelGPU), next.size * sizeof(DNvoxelGPU));

				gpuMap[DN_FLATTEN_INDEX(next.chunkPos, vol->mapSize)].voxelIndex = next.startPos - cur.size;
			}

			next.startPos -= cur.size;
			cur.startPos += next.size;

			vol->gpuVoxelLayout[i] = next;
			vol->gpuVoxelLayout[i + 1] = cur;
		}

		//merge adjavent empty nodes together:
		if(!DN_in_map_bounds(vol, next.chunkPos) && cur.size == next.size && cur.size < DN_CHUNK_LENGTH)
		{
			numCopies++;

			vol->gpuVoxelLayout[i].size *= 2;

			if(i < vol->numVoxelNodes - 2)
				memmove(&vol->gpuVoxelLayout[i + 1], &vol->gpuVoxelLayout[i + 2], sizeof(DNvoxelNode) * (vol->numVoxelNodes - i - 2));
			
			vol->numVoxelNodes--;
		}

		if(numCopies > maxCopies)
			break;
	}
}