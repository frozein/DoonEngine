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

//the maximum material index that can be used:
#define MAX_MATERIALS 256

//--------------------------------------------------------------------------------------------------------------------------------//

//a single voxel
typedef struct Voxel
{
	vec3 albedo; //the color
	vec3 normal; //the normal
	GLuint material; //the material index
} Voxel;

//a single voxel, as stored on the GPU
typedef struct VoxelGPU
{
	GLuint albedo; //compressed
	GLuint normal; //compressed
	GLuint directLight; //compressed
	GLuint specLight; //compressed
} VoxelGPU;

//a chunk of voxels, as stored on the GPU
typedef struct VoxelChunk
{
	VoxelGPU voxels[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
	uvec3 position;
	
	GLuint fill;
} VoxelChunk;

typedef struct VoxelChunkHandle
{
	GLuint flag;     //0 = does not exist, 1 = exists and loaded, 2 = exists and unloaded, 3 = exists, unloaded, and requested
	GLuint visible;  //whether or not the voxel is visible to the camera
	GLuint lastUsed; //the time since the chunk was last used
	GLuint gpuIndex; //the index of the chunk in the GPU-side chunk buffer 
	GLuint index;    //the index of the chunk
} VoxelChunkHandle;

typedef struct VoxelMaterial
{
	GLuint emissive;

	GLfloat opacity;

	GLfloat specular;
	GLuint reflectSky;
	GLuint shininess;

	vec3 fill; //needed for alignment
} VoxelMaterial;

//--------------------------------------------------------------------------------------------------------------------------------//
//NOTE: this memory may differ from that on the GPU, what gets sent to the GPU is determined automatically

extern VoxelChunkHandle* voxelMap; //The x component maps each map position to a chunk in voxelChunks
extern VoxelChunk* voxelChunks; //Every currently loaded chunk
extern VoxelMaterial* voxelMaterials; //Every currently active material
extern ivec4* voxelLightingRequests; //Every chunk requested to have its lighting updated

extern vec3 sunDir;
extern vec3 sunStrength;
extern float ambientStrength;
extern unsigned int bounceLimit;
extern float shadowSoftness;
extern unsigned int viewMode;

//--------------------------------------------------------------------------------------------------------------------------------//

//Initializes the entire voxel rendering pipeline. MUST BE CALLED BEFORE ANY OF THE OTHER FUNCTIONS. Returns true on success, false on failure
bool init_voxel_pipeline(uvec2 textureSize, Texture finalTex, uvec3 mapSize, unsigned int maxChunks, uvec3 mapSizeGPU, unsigned int maxChunksGPU, unsigned int maxLightingRequests);
//Completely cleans up and deinitializes the voxel pipeline
void deinit_voxel_pipeline();

//Updates all of the GPU-side voxel memory
void update_gpu_voxel_data();
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
uvec2 voxel_texture_size();
//Sets the current texture size. Returns true on success, false on failure
bool set_voxel_texture_size(uvec2 size);

//Returns the current map size (in chunks)
uvec3 voxel_map_size();
//Sets the current map size (in chunks). Returns true on success, false on failure
bool set_voxel_map_size(uvec3 size);

//Returns the current maximum number of chunks
unsigned int max_voxel_chunks();
//Returns the current number of chunks in use
unsigned int current_voxel_chunks();
//Sets the current maximum number of chunks. Returns true on success, false on failure
bool set_max_voxel_chunks(unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//

//Returns the current map size on the GPU (in chunks)
uvec3 voxel_map_size_gpu();
//Sets the current map size on the GPU (in chunks). Returns true on success, false on failure
bool set_voxel_map_size_gpu(uvec3 size);

//Returns the current maximum number of chunks the GPU can store at once
unsigned int max_voxel_chunks_gpu();
//Sets the current maximum number of chunks the GPU can store at once. Returns true on success, false on failure
bool set_max_voxel_chunks_gpu(unsigned int num);

//Returns the current maximum number of lighting updates the GPU can process at once (in chunks)
unsigned int max_voxel_lighting_requests();
//Sets the current maximum number of lighting updates the GPU can process at once (in chunks). Returns true on success, false on failure
bool set_max_voxel_lighting_requests(unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//

//Compresses a voxel
VoxelGPU voxel_to_voxelGPU(Voxel voxel);
//Decompresses a voxel
Voxel voxelGPU_to_voxel(VoxelGPU voxel);

#endif