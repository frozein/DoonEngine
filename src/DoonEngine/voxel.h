#ifndef DN_VOXEL_H
#define DN_VOXEL_H

#include "globals.h"
#include "math/common.h"
#include <stdbool.h>
#include <stdint.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//the size of each chunk (in voxels):
#define DN_CHUNK_SIZE ((DNuvec3){8, 8, 8})

//the maximum number of materials (NOTE: a material of 255 represents an empty voxel):
#define DN_MAX_MATERIALS 256

//flattens a 3D vector position into a 1D array index given the dimensions of the array
#define DN_FLATTEN_INDEX(p, s) (p.x) + (s.x) * ((p.y) + (p.z) * (s.y))

//--------------------------------------------------------------------------------------------------------------------------------//

//a single voxel
typedef struct DNvoxel
{
	DNvec3 normal; 	  //the normal (direction the voxel points towards)
	uint8_t material; //the index into the materials array, in the range [0, 255] (NOTE: a material of 255 represents an empty voxel)
} DNvoxel;

//a compressed voxel, this is how voxels are actually stored in memory
typedef struct DNcompressedVoxel
{
	GLuint normal; //layout: normal.x (8 bits) | normal.y (8 bits) | normal.z (8 bits) | material index (8 bits)
} DNcompressedVoxel;

//a chunk of voxels, voxels are stored this way to save memory and accelerate ray casting
typedef struct DNchunk
{
	DNivec3 pos; 	  //the chunk's position within the entire map
	GLuint used; 	  //whether the chunk is currently in use (chunks can be empty)
	GLuint updated;   //whether the chunk has updates not yet pushed to the GPU
	GLuint numVoxels; //the number of filled voxels this chunk contains, used to identify empty chunks for removal

	DNcompressedVoxel voxels[8][8][8]; //the grid of voxels in this chunk, of size DN_CHUNK_SIZE
} DNchunk;

//a handle to a voxel chunk, along with some meta-data
typedef struct DNchunkHandle
{
	GLuint flag;     //0 = does not exist, 1 = loaded on CPU but not GPU, 2 = loaded on CPU and GPU, 3 = loaded on CPU and requested on GPU
	GLuint visible;  //whether or not this map tile is visible to the camera (ACCESSIBLE ON GPU ONLY)
	GLuint lastUsed; //the time, in frames, since the chunk was last used (ACCESSIBLE ON GPU ONLY)
	GLuint index;    //the index of the chunk that this handle points to. if flag = 0, this is invalid
} DNchunkHandle;

//material properties for a voxel
typedef struct DNmaterial
{
	DNvec3 albedo;      //the "base color" of the voxel, the proportion of light reflected
	GLuint emissive;    //whether or not the voxel emits light, represented as a uint for GPU memory alignment
	GLfloat opacity;    //the voxel's opacity, in the range [0.0, 1.0]
	GLfloat specular;   //the percent of light reflected specularly by a voxel in the range [0.0, 1.0]
	GLuint reflectType; //only for materials where specular > 0.0; 0 = do not reflect the sky color, 1 = reflect the sky color, 2 = specular highlight only (much faster to calculate)
	GLuint shininess;   //only for materials where specular > 0.0; determines how perfect the reflcetions are, the greater this number, the closer to a perfect mirror
} DNmaterial;

//a voxel map, both on the CPU and the GPU
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
	unsigned int numLightingRequests; //READ ONLY | The number of chunks queued to have their lighting updated
	unsigned int lightingRequestCap;  //READ ONLY | The maximum number of chunks that can be stored in lightingRequests
	bool streamable;				  //READ ONLY | Whether or not this map supports dynamically streaming chunks to the GPU, reducing VRAM usage but reducing performance

	//data:
	DNchunkHandle* map; 		 	  //READ-WRITE | The actual map. An array with length = mapSize.x * mapSize.y * mapSize.z
	DNchunk* chunks; 	 		 	  //READ-WRITE | The array of chunks that the map has
	DNmaterial* materials;			  //READ-WRITE | The array of materials that the map has
	DNuvec4* lightingRequests;        //READ-WRITE | An array of chunk indices (represented as a uvec4 due to a need for aligment on the gpu, only the x component is used), signifies which chunks will have their lighting updated when DN_update_lighting() is called
	DNivec3* gpuChunkLayout;		  //READ ONLY  | An array representing the chunk layout on the GPU, only used for streamable maps. Represents the position in the map that each chunk is.

	//camera parameters:
	DNvec3 camPos; 			  		  //READ-WRITE | The camera's position relative to this map, in DNchunks
	DNvec3 camOrient; 		  		  //READ-WRITE | The camera's orientation, in degrees. Expressed as {pitch, yaw, roll}
	float camFOV; 			 		  //READ-WRITE | The camera's field of view
	unsigned int camViewMode; 		  //READ-WRITE | The camera's view mode, 0 = normal; 1 = albedo only; 2 = direct light only; 3 = diffuse light only; 4 = specular light only; 5 = normals

	//lighting parameters:
	DNvec3 sunDir; 					  //READ-WRITE | The direction pointing towards the sun
	DNvec3 sunStrength; 			  //READ-WRITE | The strength of sunlight, also determines the color
	DNvec3 ambientLightStrength; 	  //READ-WRITE | The minimum lighting for every voxel
	unsigned int diffuseBounceLimit;  //READ-WRITE | The maximum number of bounces for diffuse rays, can greatly affect performance
	unsigned int specBounceLimit; 	  //READ-WRITE | The maximum number of bounces for specular rays, can greatly affect performance
	float shadowSoftness; 			  //READ-WRITE | How soft shadows from direct light appear

	unsigned int frameNum;			  //READ ONLY  | Used to split the lighting calculations over multiple frames, determines the current frame. In the range [0, lightingSplit - 1]
	float lastTime;					  //READ ONLY  | Used to ensure that each group of chunks receives the same time value, even when they are calculated at different times
} DNmap;

//represents the types of chunks that can be requested for lighting updates
typedef enum DNchunkRequests
{
	DN_REQUEST_VISIBLE, //will request all visible chunks to have their lighting updated
	DN_REQUEST_LOADED,  //will request all loaded chunks to have their lighting updated
	DN_REQUEST_NONE 	//will not request any chunks, leaves the lightingRequest buffer the same as it was previously
} DNchunkRequests;

//represents memory operations
typedef enum DNmemOp
{
	DN_READ,
	DN_WRITE,
	DN_READ_WRITE
} DNmemOp;

//--------------------------------------------------------------------------------------------------------------------------------//
//INITIALIZATION:

//Initializes the entire voxel rendering pipeline. Call this before any other functions. @returns true on success, or false on failure
bool DN_init();
//Completely cleans up and deinitializes the voxel pipeline
void DN_quit();

/*Creates a new DNmap with the specified parameters 
 * @param mapSize the size, in DNchunks, of the map
 * @param textureSize the size, in pixels, of the texture that is rendered to
 * @param streamable whether or not the map will be streamable, allowing chunks to be dynamically streamed to the GPU every frame
 * @param minChunks for streamable maps only. determines the minimum number of chunks that will be loaded on the GPU. If set too low, the map may lag for the first few frames. 
 * @returns the new map or NULL if the map creation failed in any way
 */
DNmap* DN_create_map(DNuvec3 mapSize, DNuvec2 textureSize, bool streamable, unsigned int minChunks);
/* Deletes a DNmap, should be called whenever a map is no longer needed to avoid memory leaks
 * @param map the map to delete
 */
void DN_delete_map(DNmap* map);

/* Loads a DNmap from a file
 * @param filePath the path to the file to load from
 * @param textureSize the size, in pixels, of the texture that is rendered to
 * @param streamable whether or not the map will be streamable, allowing chunks to be dynamically streamed to the GPU every frame
 * @param minChunks for streamable maps only. determines the minimum number of chunks that will be loaded on the GPU. If set too low, the map may lag for the first few frames. 
 * @returns the loaded map or NULL, on failure
 */
DNmap* DN_load_map(const char* filePath, DNuvec2 textureSize, bool streamable, unsigned int minChunks);
/* Saves a DNmap to a file
 * @param filePath the path of the file to save to
 * @param map the map to save
 * @returns true on success, false on failure
 */
bool DN_save_map(const char* filePath, DNmap* map);

//--------------------------------------------------------------------------------------------------------------------------------//
//DRAWING:

/* Draws the voxel map to the texture
 * @param map the map to render
 */
void DN_draw(DNmap* map);

/* Updates the lighting on every chunk currently in a map's lightingRequests
 * @param map the map to update
 * @param numDiffuseSamples the number of diffuse lighting samples to take
 * @param time the current running time of the application, used for random seeding
 */
void DN_update_lighting(DNmap* map, unsigned int numDiffuseSamples, float time);

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY:

/* Allocates space for and creates a new chunk CPU-side. It should never be necessary to call as it is called automatically
 * @param map the map to add a new chunk to
 * @param pos the position within the map to add the chunk at
 * @returns the index to the newly created chunk
 */
unsigned int DN_add_chunk(DNmap* map, DNivec3 pos);
/* Removes and frees space for a chunk. It should never be necessary to call as it is called automatically
 * @param map the map to remove the chunk from
 * @param pos the position to remove the chunk from
 */
void DN_remove_chunk(DNmap* map, DNivec3 pos);

/* Updates the gpu-side data for a map. For non-streamable maps, this only needs to be called after edits to the map. For streamable maps, this should be called every frame (or every few frames)
 * @param map the map to sync
 * @param op the operation to perform on the map. DN_READ will only query the gpu for visible chunks. DN_WRITE will upload voxel data to the GPU if updated or requested. DN_READ_WRITE will do both
 * @param requests which chunks to request to be added into the lightingRequest buffer. See the definition of DNchunkRequests for more information
 * @param lightingSplit the number of frames to split the lighting calculation over. For example, if this is set to 5 only 1/5 of the chunks will have their lighting updated each frame, increasing performace
 */
void DN_sync_gpu(DNmap* map, DNmemOp op, DNchunkRequests requests, unsigned int lightingSplit);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP SETTINGS:

/* Sets the texture size that a map renders to
 * @param map the map to change
 * @param size the size of the new texture, in pixels
 * @returns true on success, false on failure
 */
bool DN_set_texture_size(DNmap* map, DNuvec2 size);

/* Sets a map's size
 * @param map the map to change
 * @param size the new size, in DNchunks
 * @returns true on success, false on failure
 */
bool DN_set_map_size(DNmap* map, DNuvec3 size);

/* Sets a map's maximum number of chunks. It should never be necessary to call as it is called automatically
 * @param map the map to change
 * @param num the new maximum number of chunks
 * @returns true on success, false on failure
 */
bool DN_set_max_chunks(DNmap* map, unsigned int num);
/* Sets a map's maximum number of chunks in VRAM. It should never be necessary to call as it is called automatically
 * @param map the map to change
 * @param num the new maximum number of chunks
 * @returns true on success, false on failure
 */
bool DN_set_max_chunks_gpu(DNmap* map, unsigned int num);

/* Sets the current maximum number of lighting updates the map can request at once. It should never be necessary to call as it is called automatically
 * @param map the map to change
 * @param num the new maximum number of lighting requests
 * @returns true on success, false on failure
 */
bool DN_set_max_lighting_requests(DNmap* map, unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP UTILITY:

/* Determines whether or not a given map position is inside the map bounds
 * @param map the map to check
 * @param pos the position within the map to check, measured in DNchunks
 * @returns true if pos is in the range [0, map->mapSize - 1], false if not
 */
bool DN_in_map_bounds(DNmap* map, DNivec3 pos);
/* Determines whether or not a given chunk position is inside the chunk bounds
 * @param pos the position within a chunk to check, measured in DNvoxels
 * @returns true if pos is in the range [0, DN_CHUNK_SIZE - 1], false if not
 */
bool DN_in_chunk_bounds(DNivec3 pos);

/* Gets a voxel from the map. NOTE: does NOT do any bounds checking and does NOT check if the requested chunk even exists
 * @param map the map to get the voxel from
 * @param mapPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @returns the voxel
 */
DNvoxel DN_get_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);
/* Gets a compressed voxel from the map. NOTE: does NOT do any bounds checking and does NOT check if the requested chunk even exists
 * @param map the map to get the voxel from
 * @param mapPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @returns the voxel
 */
DNcompressedVoxel DN_get_compressed_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);

/* Sets a voxel in the map. Automatically allocates memory for new chunks if needed. NOTE: does NOT do any bounds checking
 * @param map the map to set the voxel in
 * @param mapPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @param voxel the voxel to set
 */
void DN_set_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos, DNvoxel voxel);
/* Sets a voxel in the map. Automatically allocates memory for new chunks if needed. NOTE: does NOT do any bounds checking
 * @param map the map to set the voxel in
 * @param mapPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @param voxel the voxel to set
 */
void DN_set_compressed_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos, DNcompressedVoxel voxel);
/* Removes a voxel from the map. Automatically frees memory if needed. NOTE: does NOT do any bounds checking
 * @param map the map to set the voxel in
 * @param mapPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 */
void DN_remove_voxel(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);

/* Determines if a chunk exists (not empty and loaded). NOTE: does NOT do any bounds checking
 * @param map the map to check
 * @param pos the position within the map, in DNchunks, to check
 * @returns true if the chunk exists and is loaded, false otherwise
 */
bool DN_does_chunk_exist(DNmap* map, DNivec3 pos);
/* Determines if a voxel exists (material != 255). NOTE: does NOT do any bounds checking
 * @param map the map to check
 * @param mapPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @returns true if the voxel is not empty (material != 255), false otherwise
 */
bool DN_does_voxel_exist(DNmap* map, DNivec3 mapPos, DNivec3 chunkPos);

/* Casts a ray into the map and determines the voxel that was hit, if any
 * @param map the map to cast the ray into
 * @param rayDir the ray's direction
 * @param rayPos the ray's origin position
 * @param maxSteps the maximum number of steps to allow the ray to take
 * @param hitPos if a voxel is hit, populated with its position within the map, measured in DNvoxels
 * @param hitVoxel if a voxel is hit, populated with the voxel that was hit
 * @param hitNormal if a voxel is hit, populated with the normal along which the voxel was hit
 * @returns true if a voxel was hit, false otherwise
 */
bool DN_step_map(DNmap* map, DNvec3 rayDir, DNvec3 rayPos, unsigned int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal);

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

/* Separates a voxel's position into a map and a chunk position
 * @param pos the position to separate, measured in DNvoxels
 * @param mapPos populated with the voxel's chunk's position within the map, equivalent to pos / DN_CHUNK_SIZE
 * @param chunkPos populated with the voxel's position within the chunk, equivalent to pos % DN_CHUNK_SIZE
 */
void DN_separate_position(DNivec3 pos, DNivec3* mapPos, DNivec3* chunkPos);

/* Determines a camera's direction given its orientation
 * @param orient the camera's orientation, expressed as {pitch, yaw, roll}
 * @returns the direction the camera points towards
 */
DNvec3 DN_cam_dir(DNvec3 orient);

/* Compresses a voxel
 * @param voxel the voxel to compress
 * @returns the compressed voxel
 */
DNcompressedVoxel DN_compress_voxel(DNvoxel voxel);
/* Decompresses a voxel
 * @param voxel the voxel to decompress
 * @returns the decompressed voxel
 */
DNvoxel DN_decompress_voxel(DNcompressedVoxel voxel);

#endif