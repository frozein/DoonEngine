#ifndef DN_VOXEL_H
#define DN_VOXEL_H

#include "math/all.h"
#include "texture.h"
#include <stdbool.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//the size of each chunk (in voxels):
#define CHUNK_SIZE_X 8
#define CHUNK_SIZE_Y 8
#define CHUNK_SIZE_Z 8

//--------------------------------------------------------------------------------------------------------------------------------//

//a single voxel
typedef struct Voxel
{
	vec3 albedo; //the color
	GLuint material; //the material index
} Voxel;

//a single voxel, as stored on the GPU
typedef struct VoxelGPU
{
	GLuint albedo; //compressed
	GLuint directLight; //compressed

	vec2 fill; //needed to maintain alignment on the GPU
} VoxelGPU;

//a chunk of voxels, as stored on the GPU
typedef struct VoxelChunk
{
	VoxelGPU voxels[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
} VoxelChunk;

//--------------------------------------------------------------------------------------------------------------------------------//
//NOTE: this memory may differ from that on the GPU, what gets sent to the GPU is determined automatically

extern int* voxelMap; //The x component maps each map position to a chunk in voxelChunks
extern VoxelChunk* voxelChunks; //Every currently loaded chunk
extern ivec4* voxelLightingRequests; //Every chunk requested to have its lighting updated

extern vec3 sunDir;
extern float sunStrength;
extern float ambientStrength;
extern unsigned int bounceLimit;
extern float bounceStrength;
extern float shadowSoftness;
extern unsigned int viewMode;

//--------------------------------------------------------------------------------------------------------------------------------//

//Initializes the entire voxel rendering pipeline. MUST BE CALLED BEFORE ANY OF THE OTHER FUNCTIONS. Returns true on success, false on failure
bool init_voxel_pipeline(uvec2 textureSize, Texture finalTex, uvec3 mapSize, unsigned int maxChunks, uvec3 mapSizeGPU, unsigned int maxChunksGPU, unsigned int maxLightingRequests);
//Completely cleans up and deinitializes the voxel pipeline
void deinit_voxel_pipeline();

//Iterates the indirect lighting on every chunk currently in voxelLightingRequests, up to numChunks
void update_voxel_indirect_lighting(unsigned int numChunks, float time);
//Updates the indirect lighting on every chunk currently in voxelLightingRequests, up to numChunks
void update_voxel_direct_lighting(unsigned int numChunks, vec3 camPos);
//Draws the voxels to a texture and renders them to the screen
void draw_voxels(vec3 camPos, vec3 camFront, vec3 camPlaneU, vec3 camPlaneV);

//TEMPORARY
void send_all_data_temp();
//--------------------------------------------------------------------------------------------------------------------------------//

//Returns the current texture size. Returns true on success, false on failure
uvec2 texture_size();
//Sets the current texture size. Returns true on success, false on failure
bool set_texture_size(uvec2 size);

//Returns the current map size (in chunks)
uvec3 map_size();
//Sets the current map size (in chunks). Returns true on success, false on failure
bool set_map_size(uvec3 size);

//Returns the current maximum number of chunks
unsigned int max_chunks();
//Returns the current number of chunks in use
unsigned int current_chunks();
//Sets the current maximum number of chunks. Returns true on success, false on failure
bool set_max_chunks(unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//

//Returns the current map size on the GPU (in chunks)
uvec3 gpu_map_size();
//Sets the current map size on the GPU (in chunks). Returns true on success, false on failure
bool set_gpu_map_size(uvec3 size);

//Returns the current maximum number of chunks the GPU can store at once
unsigned int max_gpu_chunks();
//Sets the current maximum number of chunks the GPU can store at once. Returns true on success, false on failure
bool set_max_gpu_chunks(unsigned int num);

//Returns the current maximum number of lighting updates the GPU can process at once (in chunks)
unsigned int max_lighting_requests();
//Sets the current maximum number of lighting updates the GPU can process at once (in chunks). Returns true on success, false on failure
bool set_max_lighting_requests(unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//

//Compresses a voxel
VoxelGPU voxel_to_voxelGPU(Voxel voxel);
//Decompresses a voxel
Voxel voxelGPU_to_voxel(VoxelGPU voxel);

#endif