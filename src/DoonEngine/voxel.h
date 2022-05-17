#ifndef DN_VOXEL_H
#define DN_VOXEL_H

#include "globals.h"
#include "math/common.h"
#include <stdbool.h>
#include <stdint.h>

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
	uint16_t mask; //allows you to give each voxel an identifier, perhaps to determine what object it is a part of
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
	DNuvec4 pos;

	GLuint updated;
	GLuint numVoxels;
	DNuvec2 padding;

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
extern DNvoxelMaterial* dnVoxelMaterials;

//A structure representing a voxel map, both on the CPU and the GPU
typedef struct DNmap
{
	//TODO: ADD AUTORESIZE CONTROLS
	
	//opengl handles:
	GLuint glTextureID;     		 //READ ONLY | The openGL texture ID, use glActivate() with this to render it
	GLuint glMapBufferID;   		 //READ ONLY | The openGL buffer ID for the map buffer on the GPU
	GLuint glChunkBufferID;			 //READ ONLY | The openGL buffer ID for the chunk buffer in the GPU

	//data parameters:
	DNuvec3 mapSize; 	    		 //READ ONLY | The size, in DNchunks, of the map
	DNuvec2 textureSize; 			 //READ ONLY | The size, in pixels, of the texture that the map renders to
	unsigned int chunkCap;			 //READ ONLY | The current number of DNchunks that are stored CPU-side by this map. The length of chunks
	unsigned int nextChunk;			 //READ ONLY | The next known empty chunk index. Used to speed up adding new chunks
	unsigned int numLightingRequest; //READ ONLY | The number of chunks to have their lighting updated
	unsigned int lightingRequestCap; //READ ONLY | The maximum number of chunks that can be stored in lightingRequests
	bool streamable;				 //READ ONLY | Whether or not this map supports dynamically streaming chunks to the GPU, reducing VRAM usage while reducing performance
	float streamIndex;				 //READ ONLY | If the map is streamable, specifies how many chunks, in relation to the number of visible chunks, must be stored. For example, if 100 chunks are currently being used by the GPU, and this value is set to 1.5, 150 chunks will be stored on the GPU. Higher values may increase perforance at the cost of VRAM

	//data:
	DNvoxelChunkHandle* map; 		 //READ-WRITE | A pointer to the actual map. An array with length = mapSize.x * mapSize.y * mapSize.z
	DNvoxelChunk* chunks; 	 		 //READ-WRITE | A pointer to the array of chunks that the map has
	DNuvec4* lightingRequests;       //READ-WRITE | A pointer to an array of chunk indices (represented as a uvec4 due to a need for aligment on the gpu, only the x component is used), signifies which chunks will have their lighting updated when DN_update_lighting() is called

	//camera parameters:
	DNvec3 camPos; 			  		 //READ-WRITE | The camera's position relative to this map, in DNchunks
	DNvec3 camOrient; 		  		 //READ-WRITE | The camera's orientation, in degrees. Expressed as {pitch, yaw, roll}
	float camFOV; 			 		 //READ-WRITE | The camera's field of view
	unsigned int camViewMode; 		 //READ-WRITE | The camera's view mode, 0 = normal; 1 = albedo only; 2 = direct light only; 3 = diffuse light only; 4 = specular light only; 5 = normals

	//lighting parameters:
	DNvec3 sunDir; 					 //READ-WRITE | The direction towards the sun
	DNvec3 sunStrength; 			 //READ-WRITE | The strength of sunlight, also determines the color
	float ambientLightStrength; 	 //READ-WRITE | The minimum lighting for every voxel
	unsigned int diffuseBounceLimit; //READ-WRITE | The maximum number of bounces for diffuse rays, can greatly affect performance
	unsigned int specBounceLimit; 	 //READ-WRITE | The maximum number of bounces for specular rays, can greatly affect performance
	float shadowSoftness; 			 //READ-WRITE | How soft shadows from direct light appear
} DNmap;

typedef enum DNchunkRequests
{
	DN_REQUEST_VISIBLE,
	DN_REQUEST_LOADED,
	DN_REQUEST_NONE
} DNchunkRequests;

typedef enum DNmemOp
{
	DN_READ,
	DN_WRITE,
	DN_READ_WRITE
} DNmemOp;

//--------------------------------------------------------------------------------------------------------------------------------//
//INITIALIZATION:

//Initializes the entire voxel rendering pipeline. MUST BE CALLED BEFORE ANY OF THE OTHER FUNCTIONS. Returns true on success, false on failure
bool DN_init_voxel_pipeline();
//Completely cleans up and deinitializes the voxel pipeline
void DN_deinit_voxel_pipeline();

//Creates a new DNmap with the specified parameters. For streamable chunks, minChunks determines the minimum number of chunks that will be loaded on the GPU. If set too low, the map may lag for the first few frames. Returns NULL if the map creation failed in any way
DNmap* DN_create_map(DNuvec3 mapSize, DNuvec2 textureSize, bool streamable, float streamIndex, unsigned int minChunks);
//Deletes a DNmap, should be called whenever a map is no longer needed to avoid memory leaks
void DN_delete_map(DNmap* map);

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY:

//Call in order to allocate space for a chunk. Should be called whenever a chunk in a non-streamable map is no longer empty (flag was 0). Returns the index to the new chunk.
unsigned int DN_add_chunk(DNmap* map, DNivec3 pos);
//Call in order to free space for a chunk. Should be called whenever a chunk in a non-streamable map is newly empty (flag was 1). Call to avoid memory leaks
void DN_remove_chunk(DNmap* map, DNivec3 pos);

//Updates the gpu-side data for a map. For non-streamable maps, this should be called after edits to the map. For streamable maps, this should be called every frame (or every few frames). requests determines what chunks, if any, will be placed into this map's lightingRequests array.
void DN_sync_gpu(DNmap* map, DNmemOp op, DNchunkRequests requests);

//--------------------------------------------------------------------------------------------------------------------------------//
//UPDATING/DRAWING:

//Updates all of the GPU-side voxel memory, if autoResize is true, this will automatically resize the gpu-side chunkBuffer and max lighting requests to fit the number needed
//unsigned int DN_stream_voxel_chunks(bool updateLighting, bool autoResize);
//Sets a single chunk to be updated on the GPU. Call when a chunk has been edited. camPos is needed so lighting can be updated instantly (if bool is set)
//void DN_update_voxel_chunk(DNivec3* positions, unsigned int num, bool updateLighting, DNvec3 camPos, float time);

//Draws the voxels to the texture
void DN_draw_voxels(DNmap* map);

//Updates the lighting on every chunk currently in a map's lightingRequests. If desired, you can supply an offset and maximum number of chunks to update, otherwise, set these parameters to 0. The current time must also be supplied
void DN_update_voxel_lighting(DNmap* map, unsigned int offset, unsigned int num, float time);

//--------------------------------------------------------------------------------------------------------------------------------//
//CPU-SIDE MAP SETTINGS:

//Returns the current texture size. Returns true on success, false on failure
//DNuvec2 DN_voxel_texture_size();
//Sets the current texture size. Returns true on success, false on failure
//bool DN_set_voxel_texture_size(DNuvec2 size);

//Returns the current map size (in chunks)
//DNuvec3 DN_voxel_map_size();
//Sets the current map size (in chunks). Returns true on success, false on failure
//bool DN_set_voxel_map_size(DNuvec3 size);

//Returns the current maximum number of chunks
//unsigned int DN_max_voxel_chunks();
//Returns the current number of chunks in use
//unsigned int DN_current_voxel_chunks();

//Sets the current maximum number of chunks. Returns true on success, false on failure
bool DN_set_max_chunks(DNmap* map, unsigned int num);

//Sets the current maximum number of lighting updates the map can hold at once. Returns true on success, false on failure
bool DN_set_max_lighting_requests(DNmap* map, unsigned int num);

//Returns whether or not auto resize is enabled for 
//bool DN_voxel_auto_chunk_resize();

//void DN_set_voxel_auto_chunk_resize();

//--------------------------------------------------------------------------------------------------------------------------------//
//GPU-SIDE MAP SETTINGS:

//Returns the current map size on the GPU (in chunks)
//DNuvec3 DN_voxel_map_size_gpu();
//Sets the current map size on the GPU (in chunks). Returns true on success, false on failure
//bool DN_set_voxel_map_size_gpu(DNuvec3 size);

//Returns the current maximum number of chunks the GPU can store at once
//unsigned int DN_max_voxel_chunks_gpu();
//Sets the current maximum number of chunks the GPU can store at once. Returns true on success, false on failure
//bool DN_set_max_voxel_chunks_gpu(unsigned int num);

//Returns the current maximum number of lighting updates the GPU can process at once (in chunks)
//unsigned int DN_max_voxel_lighting_requests();
//Sets the current maximum number of lighting updates the GPU can process at once (in chunks). Returns true on success, false on failure
//bool DN_set_max_voxel_lighting_requests(unsigned int num);

//Sets the voxel lighting parameters, controls how the scene looks overall
//void DN_set_voxel_lighting_parameters(DNvec3 sunDir, DNvec3 sunStrength, float ambientLightStrength, unsigned int diffuseBounceLimit, unsigned int specBounceLimit, float shadowSoftness);

//Uploads materials from dnMaterials to the gpu so their effects can be visually seen
void DN_set_voxel_materials(DNmaterialHandle min, unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP UTILITY:

//Returns whether or not a given map position is inside the map bounds
bool DN_in_map_bounds(DNmap* map, DNivec3 pos);
//Returns whether or not a given chunk position is inside the chunk bounds
bool DN_in_chunk_bounds(DNivec3 pos);

//Returns a voxel from the map. Does NOT do any bounds checking. Does NOT check if the requested chunk is loaded.
DNvoxel DN_get_voxel(DNmap* map, DNivec3 chunkPos, DNivec3 localPos);
//Identical to DN_get_voxel(), but returns a compressed voxel instead. Does NOT do any bounds checking. Does NOT check if the requested chunk is loaded.
DNvoxelGPU DN_get_compressed_voxel(DNmap* map, DNivec3 chunkPos, DNivec3 localPos);

//Sets a voxel in the map, may cause memory allocations. Does NOT do any bounds checking.
void DN_set_voxel(DNmap* map, DNivec3 chunkPos, DNivec3 localPos, DNvoxel voxel);
//Identical to DN_set_voxel(), but allows you to pass in a compressed voxel instead. Does NOT do any bounds checking.
void DN_set_compressed_voxel(DNmap* map, DNivec3 chunkPos, DNivec3 localPos, DNvoxelGPU voxel);
//Removes a voxel from the map. Does NOT do any bounds checking.
void DN_remove_voxel(DNmap* map, DNivec3 chunkPos, DNivec3 localPos);

//Returns true if the chunk at the given position is loaded, false otherwise. Does NOT do any bounds checking.
bool DN_does_chunk_exist(DNmap* map, DNivec3 pos);
//Returns true if the voxel at the given position is not empty, false otherwise. Does NOT do any bounds checking. Does NOT check if the requested chunk is loaded.
bool DN_does_voxel_exist(DNmap* map, DNivec3 chunkPos, DNivec3 localPos);

//Casts a ray into the voxel map and returns whether or not a voxel was hit. If one was hit, data about it is stored in the pointer parameters
bool DN_step_voxel_map(DNmap* map, DNvec3 rayDir, DNvec3 rayPos, unsigned int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal);

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

//Separates a global position into a chunk and local position. chunkPos = pos / CHUNK_SIZE, localPos = pos % CHUNK_SIZE
void DN_separate_position(DNivec3 pos, DNivec3* chunkPos, DNivec3* localPos);

//Compresses a voxel
DNvoxelGPU DN_voxel_to_voxelGPU(DNvoxel voxel);
//Decompresses a voxel
DNvoxel DN_voxelGPU_to_voxel(DNvoxelGPU voxel);

#endif