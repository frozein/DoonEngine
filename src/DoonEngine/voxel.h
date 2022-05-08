#ifndef DN_VOXEL_H
#define DN_VOXEL_H

#include "globals.h"
#include "math/common.h"
#include "utility/texture.h"
#include <stdbool.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//the size of each chunk (in voxels):
#define DN_CHUNK_SIZE ((DNuvec3){8, 8, 8})

//the maximum material index that can be used:
#define DN_MAX_VOXEL_MATERIALS 256

//--------------------------------------------------------------------------------------------------------------------------------//

//a single voxel
typedef struct DNvoxel
{
	DNvec3 albedo; //the color
	DNvec3 normal; //the normal
	GLuint material; //the material index
} DNvoxel;

//a single voxel, as stored on the GPU
typedef struct DNvoxelGPU
{
	GLuint albedo;         //compressed
	GLuint normal;         //compressed
	GLuint directLight;    //compressed
	GLuint specLight;      //compressed
} DNvoxelGPU;

//a chunk of voxels, as stored on the GPU
typedef struct DNvoxelChunk
{
	DNvoxelGPU voxels[8][8][8]; //CHUNK SIZE = 8
	DNvec4 indirectLight[8][8][8];
} DNvoxelChunk;

//a handle to a voxel chunk, along with some meta-data
typedef struct DNvoxelChunkHandle
{
	GLuint flag;     //0 = does not exist, 1 = exists and loaded, 2 = exists and unloaded, 3 = exists, unloaded, and requested
	GLuint visible;  //whether or not the voxel is visible to the camera
	GLuint lastUsed; //the time since the chunk was last used
	GLuint index;    //the index of the chunk (cpu or gpu side depending on where the handle came from)
} DNvoxelChunkHandle;

//material properties for a voxel
typedef struct DNvoxelMaterial
{
	GLuint emissive;

	GLfloat opacity;

	GLfloat specular;
	GLuint reflectSky;
	GLuint shininess;

	DNvec3 fill; //needed for alignment
} DNvoxelMaterial;

typedef unsigned char DNmaterialHandle;

//--------------------------------------------------------------------------------------------------------------------------------//
//NOTE: this memory may differ from that on the GPU, what gets sent to the GPU is determined automatically

extern DNvoxelChunkHandle* dnVoxelMap; //The x component maps each map position to a chunk in dnVoxelChunks
extern DNvoxelChunk* dnVoxelChunks; //Every currently loaded chunk
extern DNvoxelMaterial* dnVoxelMaterials; //Every currently active material
extern DNivec4* dnVoxelLightingRequests; //Every chunk requested to have its lighting updated

//--------------------------------------------------------------------------------------------------------------------------------//
//INITIALIZATION:

//Initializes the entire voxel rendering pipeline. MUST BE CALLED BEFORE ANY OF THE OTHER FUNCTIONS. Returns true on success, false on failure
bool DN_init_voxel_pipeline(DNuvec2 textureSize, DNtexture finalTex, DNuvec3 mapSize, unsigned int maxChunks, DNuvec3 mapSizeGPU, unsigned int maxChunksGPU, unsigned int maxLightingRequests);
//Completely cleans up and deinitializes the voxel pipeline
void DN_deinit_voxel_pipeline();

//--------------------------------------------------------------------------------------------------------------------------------//
//UPDATING/DRAWING:

//Updates all of the GPU-side voxel memory, if autoResize is true, this will automatically resize the gpu-side chunkBuffer and max lighting requests to fit the number needed
unsigned int DN_stream_voxel_chunks(bool updateLighting, bool autoResize);
//Sets a single chunk to be updated on the GPU. Call when a chunk has been edited. camPos is needed so lighting can be updated instantly (if bool is set)
void DN_update_voxel_chunk(DNivec3* positions, unsigned int num, bool updateLighting, DNvec3 camPos, float time);

//Draws the voxels to the texture
void DN_draw_voxels(DNvec3 camPos, float fov, DNvec3 angle, unsigned int viewMode);

//Updates the indirect lighting on every chunk currently in dnVoxelLightingRequests, up to numChunks
void DN_update_voxel_lighting(unsigned int numChunks, unsigned int offset, DNvec3 camPos, float time);

//--------------------------------------------------------------------------------------------------------------------------------//
//CPU-SIDE MAP SETTINGS:

//Returns the current texture size. Returns true on success, false on failure
DNuvec2 DN_voxel_texture_size();
//Sets the current texture size. Returns true on success, false on failure
bool DN_set_voxel_texture_size(DNuvec2 size);

//Returns the current map size (in chunks)
DNuvec3 DN_voxel_map_size();
//Sets the current map size (in chunks). Returns true on success, false on failure
bool DN_set_voxel_map_size(DNuvec3 size);

//Returns the current maximum number of chunks
unsigned int DN_max_voxel_chunks();
//Returns the current number of chunks in use
unsigned int DN_current_voxel_chunks();
//Sets the current maximum number of chunks. Returns true on success, false on failure
bool DN_set_max_voxel_chunks(unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//
//GPU-SIDE MAP SETTINGS:

//Returns the current map size on the GPU (in chunks)
DNuvec3 DN_voxel_map_size_gpu();
//Sets the current map size on the GPU (in chunks). Returns true on success, false on failure
bool DN_set_voxel_map_size_gpu(DNuvec3 size);

//Returns the current maximum number of chunks the GPU can store at once
unsigned int DN_max_voxel_chunks_gpu();
//Sets the current maximum number of chunks the GPU can store at once. Returns true on success, false on failure
bool DN_set_max_voxel_chunks_gpu(unsigned int num);

//Returns the current maximum number of lighting updates the GPU can process at once (in chunks)
unsigned int DN_max_voxel_lighting_requests();
//Sets the current maximum number of lighting updates the GPU can process at once (in chunks). Returns true on success, false on failure
bool DN_set_max_voxel_lighting_requests(unsigned int num);

//Sets the voxel lighting parameters, controls how the scene looks overall
void DN_set_voxel_lighting_parameters(DNvec3 sunDir, DNvec3 sunStrength, float ambientLightStrength, unsigned int diffuseBounceLimit, unsigned int specBounceLimit, float shadowSoftness);

//Sends all of the materials in the given range (inclusive), needed to see materials visually updated
void DN_set_voxel_materials(DNmaterialHandle min, DNmaterialHandle max);

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

//Compresses a voxel
DNvoxelGPU DN_voxel_to_voxelGPU(DNvoxel voxel);
//Decompresses a voxel
DNvoxel DN_voxelGPU_to_voxel(DNvoxelGPU voxel);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP UTILITY:

//Returns whether or not a given map position is inside the map bounds
bool DN_in_voxel_map_bounds(DNivec3 pos);
//Returns whether or not a given chunk position is inside the chunk bounds
bool DN_in_voxel_chunk_bounds(DNivec3 pos);
//Returns the map tile at the given map position. DOESN'T DO ANY BOUNDS CHECKING
DNvoxelChunkHandle DN_get_voxel_chunk_handle(DNivec3 pos);
//Returns the voxel at the given chunk position. DOESN'T DO ANY BOUNDS CHECKING
DNvoxelGPU DN_get_voxel(unsigned int chunkIndex, DNivec3 pos); //returns the voxel of a chunk at a position DOESNT DO ANY BOUNDS CHECKING
//Returns whether or not the voxel at the given chunk position is solid 
bool DN_does_voxel_exist(unsigned int chunkIndex, DNivec3 localPos);

//Casts a ray into the voxel map and returns whether or not a voxel was hit. If one was hit, data about it is stored in the pointer parameters
bool DN_step_voxel_map(DNvec3 rayDir, DNvec3 rayPos, unsigned int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal);

#endif