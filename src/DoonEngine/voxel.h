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
	GLuint albedo;         //compressed
	GLuint normal;         //compressed
	GLuint directLight;    //compressed
	GLuint specLight;      //compressed
} VoxelGPU;

//a chunk of voxels, as stored on the GPU
typedef struct VoxelChunk
{
	VoxelGPU voxels[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
	vec4 indirectLight[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
} VoxelChunk;

typedef struct VoxelChunkHandle
{
	GLuint flag;     //0 = does not exist, 1 = exists and loaded, 2 = exists and unloaded, 3 = exists, unloaded, and requested
	GLuint visible;  //whether or not the voxel is visible to the camera
	GLuint lastUsed; //the time since the chunk was last used
	GLuint index;    //the index of the chunk (cpu or gpu side depending on where the handle came from)
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
//INITIALIZATION:

//Initializes the entire voxel rendering pipeline. MUST BE CALLED BEFORE ANY OF THE OTHER FUNCTIONS. Returns true on success, false on failure
bool init_voxel_pipeline(uvec2 textureSize, Texture finalTex, uvec3 mapSize, unsigned int maxChunks, uvec3 mapSizeGPU, unsigned int maxChunksGPU, unsigned int maxLightingRequests);
//Completely cleans up and deinitializes the voxel pipeline
void deinit_voxel_pipeline();

//--------------------------------------------------------------------------------------------------------------------------------//
//UPDATING/DRAWING:

//Updates all of the GPU-side voxel memory
unsigned int stream_voxel_chunks(bool updateLighting);
//Sets a single chunk to be updated on the GPU. Call when a chunk has been edited. camPos is needed so lighting can be updated instantly (if bool is set)
void update_voxel_chunk(ivec3* positions, int num, bool updateLighting, vec3 camPos);

//Draws the voxels to a texture and renders them to the screen
void draw_voxels(vec3 camPos, vec3 camFront, vec3 camPlaneU, vec3 camPlaneV);

//Iterates the indirect lighting on every chunk currently in voxelLightingRequests, up to numChunks
void update_voxel_indirect_lighting(unsigned int numChunks, unsigned int offset, float time);
//Updates the indirect lighting on every chunk currently in voxelLightingRequests, up to numChunks
void update_voxel_direct_lighting(unsigned int numChunks, unsigned int offset, vec3 camPos);

//TEMPORARY
void send_all_data_temp();

//--------------------------------------------------------------------------------------------------------------------------------//
//CPU-SIDE SETTINGS:

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
//GPU-SIDE SETTINGS

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
//UTILITY:

//Returns whether or not a given map position is inside the map bounds
bool in_map_bounds(ivec3 pos);
//Returns whether or not a given chunk position is inside the chunk bounds
bool in_chunk_bounds(ivec3 pos);
//Returns the map tile at the given map position. DOESN'T DO ANY BOUNDS CHECKING
VoxelChunkHandle get_map_tile(ivec3 pos);
//Returns the voxel at the given chunk position. DOESN'T DO ANY BOUNDS CHECKING
VoxelGPU get_voxel(unsigned int chunk, ivec3 pos); //returns the voxel of a chunk at a position DOESNT DO ANY BOUNDS CHECKING
//Returns whether or not the voxel at the given chunk position is solid 
bool does_voxel_exist(unsigned int chunk, ivec3 localPos);

//Casts a ray into the voxel map and returns whether or not a voxel was hit. If one was hit, data about it is stored in the pointer parameters
bool step_voxel_map(vec3 rayDir, vec3 rayPos, unsigned int maxSteps, ivec3* hitPos, Voxel* hitVoxel, ivec3* hitNormal);

//Compresses a voxel
VoxelGPU voxel_to_voxelGPU(Voxel voxel);
//Decompresses a voxel
Voxel voxelGPU_to_voxel(VoxelGPU voxel);

#endif