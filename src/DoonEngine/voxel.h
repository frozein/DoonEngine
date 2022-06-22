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

//flattens a 3D vector position into a 1D array index given the dimensions of the array
#define DN_FLATTEN_INDEX(p, s) (p.x) + (s.x) * ((p.y) + (p.z) * (s.y))

//--------------------------------------------------------------------------------------------------------------------------------//

//a single voxel
typedef struct DNvoxel
{
	DNvec3 normal; 	  //the normal
	uint8_t material; //the material index, rangine from 0-255, a material of 255 represents an empty voxel
} DNvoxel;

//a compressed voxel
typedef struct DNcompressedVoxel
{
	GLuint normal; //layout: normal.x (8 bits) | normal.y (8 bits) | normal.z (8 bits) | material index (8 bits)
} DNcompressedVoxel;

//a chunk of compressed voxels
typedef struct DNchunk
{
	DNivec3 pos; 	  //the chunk's position within the entire map
	GLuint used; 	  //whether the chunk is currently used by a map tile
	GLuint updated;   //whether the chunk has updates not yet pushed to the GPU
	GLuint numVoxels; //the number of filled voxels this chunk contains, used to identify empty chunks for removal

	DNcompressedVoxel voxels[8][8][8]; //the grid of voxels in this map, of size CHUNK_SIZE
} DNchunk;

//a handle to a voxel chunk, along with some meta-data
typedef struct DNchunkHandle
{
	GLuint flag;     //0 = does not exist, 1 = loaded on CPU but not GPU, 2 = loaded on CPU and GPU, 3 = loaded on CPU and requested on GPU
	GLuint visible;  //whether or not this map tile is visible to the camera
	GLuint lastUsed; //the time, in frames, since the chunk was last used
	GLuint index;    //the index of the corresponding chunk (cpu or gpu side depending on where the handle came from)
} DNchunkHandle;

//material properties for a voxel
typedef struct DNmaterial
{
	DNvec3 albedo;      //the "base color" of the voxel, the proportion of light reflected
	GLuint emissive;    //whether or not the voxel emits light
	GLfloat opacity;    //the voxel's opacity, from 0.0 to 1.0
	GLfloat specular;   //the percent of light reflected specularly by a voxel, from 0.0 to 1.0
	GLuint reflectType; //only for materials where specular > 0.0; 0 = do not reflect the sky color, 1 = reflect the sky color, 2 = highlight only (much faster to calculate)
	GLuint shininess;   //only for materials where specular > 0.0; determines how perfect the reflcetions are, the greater this number, the closer to a perfect mirror
} DNmaterial;

typedef unsigned char DNmaterialHandle;
extern DNmaterial* dnMaterials; //the list of all materials used by the engine, can be indexed up to 254 (a material of 255 indicated an empty voxel)

//A structure representing a voxel map, both on the CPU and the GPU
typedef struct DNmap
{	
	//opengl handles:
	GLuint glTextureID;     		  //READ ONLY | The openGL texture ID, use glActivate() with this to render it
	GLuint glMapBufferID;   		  //READ ONLY | The openGL buffer ID for the map buffer on the GPU
	GLuint glChunkBufferID;			  //READ ONLY | The openGL buffer ID for the chunk buffer in the GPU

	//data parameters:
	DNuvec3 mapSize; 	    		  //READ ONLY | The size, in DNchunks, of the map
	DNuvec2 textureSize; 			  //READ ONLY | The size, in pixels, of the texture that the map renders to
	unsigned int chunkCap;			  //READ ONLY | The current number of DNchunks that are stored CPU-side by this map. The length of chunks
	unsigned int chunkCapGPU; 		  //READ ONLY | The current number of DNchunks that are stored GPU-side bu this map.
	unsigned int nextChunk;			  //READ ONLY | The next known empty chunk index. Used to speed up adding new chunks
	unsigned int numLightingRequests; //READ ONLY | The number of chunks to have their lighting updated
	unsigned int lightingRequestCap;  //READ ONLY | The maximum number of chunks that can be stored in lightingRequests
	bool streamable;				  //READ ONLY | Whether or not this map supports dynamically streaming chunks to the GPU, reducing VRAM usage while reducing performance

	//data:
	DNchunkHandle* map; 		 	  //READ-WRITE | A pointer to the actual map. An array with length = mapSize.x * mapSize.y * mapSize.z
	DNchunk* chunks; 	 		 	  //READ-WRITE | A pointer to the array of chunks that the map has
	DNmaterial* materials;			  //READ-WRITE | A pointer to the array of materials that the map has
	DNuvec4* lightingRequests;        //READ-WRITE | A pointer to an array of chunk indices (represented as a uvec4 due to a need for aligment on the gpu, only the x component is used), signifies which chunks will have their lighting updated when DN_update_lighting() is called
	DNivec3* gpuChunkLayout;		  //READ ONLY  | A pointer to an array representing the chunk layout on the GPU, only used for streamable maps. Represents the position in the map that each chunk is.

	//camera parameters:
	DNvec3 camPos; 			  		  //READ-WRITE | The camera's position relative to this map, in DNchunks
	DNvec3 camOrient; 		  		  //READ-WRITE | The camera's orientation, in degrees. Expressed as {pitch, yaw, roll}
	float camFOV; 			 		  //READ-WRITE | The camera's field of view
	unsigned int camViewMode; 		  //READ-WRITE | The camera's view mode, 0 = normal; 1 = albedo only; 2 = direct light only; 3 = diffuse light only; 4 = specular light only; 5 = normals

	//lighting parameters:
	DNvec3 sunDir; 					  //READ-WRITE | The direction towards the sun
	DNvec3 sunStrength; 			  //READ-WRITE | The strength of sunlight, also determines the color
	DNvec3 ambientLightStrength; 	  //READ-WRITE | The minimum lighting for every voxel
	unsigned int diffuseBounceLimit;  //READ-WRITE | The maximum number of bounces for diffuse rays, can greatly affect performance
	unsigned int specBounceLimit; 	  //READ-WRITE | The maximum number of bounces for specular rays, can greatly affect performance
	float shadowSoftness; 			  //READ-WRITE | How soft shadows from direct light appear

	unsigned int frameNum;			  //READ ONLY  | Used to split the lighting calculations over multiple frames, determines the current frame. In the range [0, lightingSplit - 1]
	float lastTime;					  //READ ONLY  | Used to ensure that each group of chunks receives the same time value, even when they are calculated at different times
} DNmap;

//An enumeration representing the types of chunks that can be requested for lighting updates
typedef enum DNchunkRequests
{
	DN_REQUEST_VISIBLE, //will request all visible chunks to have their lighting updated
	DN_REQUEST_LOADED,  //will request all loaded chunks to have their lighting updated
	DN_REQUEST_NONE 	//will not request any chunks, leaves the lightingRequest buffer the same as it was previously
} DNchunkRequests;

//An enumeration representing memory operations
typedef enum DNmemOp
{
	DN_READ,
	DN_WRITE,
	DN_READ_WRITE
} DNmemOp;

//--------------------------------------------------------------------------------------------------------------------------------//
//INITIALIZATION:

//Initializes the entire voxel rendering pipeline. MUST BE CALLED BEFORE ANY OF THE OTHER FUNCTIONS. Returns true on success, false on failure
bool DN_init();
//Completely cleans up and deinitializes the voxel pipeline
void DN_quit();

//Creates a new DNmap with the specified parameters. For streamable chunks, minChunks determines the minimum number of chunks that will be loaded on the GPU. If set too low, the map may lag for the first few frames. Returns NULL if the map creation failed in any way
DNmap* DN_create_map(DNuvec3 mapSize, DNuvec2 textureSize, bool streamable, unsigned int minChunks);
//Deletes a DNmap, should be called whenever a map is no longer needed to avoid memory leaks
void DN_delete_map(DNmap* map);

//Loads a DNmap from a file with the specified parameters. @returns the loaded map or NULL, on failure
DNmap* DN_load_map(const char* filePath, DNuvec2 textureSize, bool streamable, unsigned int minChunks);
//Saves a DNmap to a file. @returns true on success, false on failure
bool DN_save_map(const char* filePath, DNmap* map);

//--------------------------------------------------------------------------------------------------------------------------------//
//DRAWING:

//Draws the voxels to the texture
void DN_draw(DNmap* map);

//Updates the lighting on every chunk currently in a map's lightingRequests. @param split the number of frames it takes up update every chunk @param numDiffuseSamples the number of diffuse lighting samples to take
void DN_update_lighting(DNmap* map, unsigned int numDiffuseSamples, float time);

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY:

//Call in order to allocate space for a chunk. Should be called whenever a chunk in a non-streamable map is no longer empty (flag was 0). Returns the index to the new chunk.
unsigned int DN_add_chunk(DNmap* map, DNivec3 pos);
//Call in order to free space for a chunk. Should be called whenever a chunk in a non-streamable map is newly empty (flag was 1). Call to avoid memory leaks
void DN_remove_chunk(DNmap* map, DNivec3 pos);

//Updates the gpu-side data for a map. For non-streamable maps, this should be called after edits to the map. For streamable maps, this should be called every frame (or every few frames). @param requests determines what chunks, if any, will be placed into this map's lightingRequests array.
void DN_sync_gpu(DNmap* map, DNmemOp op, DNchunkRequests requests, unsigned int lightingSplit);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP SETTINGS:

//Sets the current texture size. Returns true on success, false on failure
bool DN_set_texture_size(DNmap* map, DNuvec2 size);

//Sets the current map size (in chunks). Returns true on success, false on failure
bool DN_set_map_size(DNmap* map, DNuvec3 size);

//Sets the current maximum number of chunks. Returns true on success, false on failure
bool DN_set_max_chunks(DNmap* map, unsigned int num);
//Sets the current macimum number of chunks on the GPU. Returns true on success, false on failure
bool DN_set_max_chunks_gpu(DNmap* map, unsigned int num);

//Sets the current maximum number of lighting updates the map can hold at once. Returns true on success, false on failure
bool DN_set_max_lighting_requests(DNmap* map, unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP UTILITY:

//NOTE: mapPos represents a voxel's position within the map [0, mapSize], chunkPos represents the position within the chunk [0, DN_CHUNK_SIZE]

//Returns whether or not a given map position is inside the map bounds
bool DN_in_map_bounds(DNmap* map, DNivec3 pos);
//Returns whether or not a given chunk position is inside the chunk bounds
bool DN_in_chunk_bounds(DNivec3 pos);

//Returns a voxel from the map. Does NOT do any bounds checking. Does NOT check if the requested chunk is loaded.
DNvoxel DN_get_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);
//Identical to DN_get_voxel(), but returns a compressed voxel instead. Does NOT do any bounds checking. Does NOT check if the requested chunk is loaded.
DNcompressedVoxel DN_get_compressed_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);

//Sets a voxel in the map, may cause memory allocations. Does NOT do any bounds checking.
void DN_set_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos, DNvoxel voxel);
//Identical to DN_set_voxel(), but allows you to pass in a compressed voxel instead. Does NOT do any bounds checking.
void DN_set_compressed_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos, DNcompressedVoxel voxel);
//Removes a voxel from the map. Does NOT do any bounds checking.
void DN_remove_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);

//Returns true if the chunk at the given position is loaded, false otherwise. Does NOT do any bounds checking.
bool DN_does_chunk_exist(DNmap* map, DNivec3 pos);
//Returns true if the voxel at the given position is not empty, false otherwise. Does NOT do any bounds checking. Does NOT check if the requested chunk is loaded.
bool DN_does_voxel_exist(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);

//Casts a ray into the voxel map and returns whether or not a voxel was hit. If one was hit, data about it is stored in the pointer parameters
bool DN_step_map(DNmap* map, DNvec3 rayDir, DNvec3 rayPos, unsigned int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal);

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

//Separates a global position into a chunk and local position. mapPos = pos / CHUNK_SIZE, chunkPos = pos % CHUNK_SIZE
void DN_separate_position(DNivec3 pos, DNivec3* mapPos, DNivec3* chunkPos);

//Returns the camera's direction given its orientation (expressed as {pitch, yaw, roll}).
DNvec3 DN_cam_dir(DNvec3 orient);

//Compresses a voxel
DNcompressedVoxel DN_compress_voxel(DNvoxel voxel);
//Decompresses a voxel
DNvoxel DN_decompress_voxel(DNcompressedVoxel voxel);

#endif