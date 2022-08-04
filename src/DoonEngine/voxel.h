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

//the value used for gamma correction, raise albedo values to this value to convert them to linear color space
#define DN_GAMMA 2.2f

//flattens a 3D vector position into a 1D array index given the dimensions of the array
#define DN_FLATTEN_INDEX(p, s) (p.x) + (s.x) * ((p.y) + (p.z) * (s.y))

//--------------------------------------------------------------------------------------------------------------------------------//

//a single voxel
typedef struct DNvoxel
{
	DNvec3 normal; 	  //the normal (direction the voxel points towards)
	uint8_t material; //the index into the materials array, in the range [0, 255] (NOTE: a material of 255 represents an empty voxel)
	DNvec3 albedo;    //the "base color" (the percentage of light that gets reflected)
} DNvoxel;

//a compressed voxel, this is how voxels are actually stored in memory
typedef struct DNcompressedVoxel
{
	GLuint normal; //layout: normal.x (8 bits) | normal.y (8 bits) | normal.z (8 bits) | material index (8 bits)
	GLuint albedo; //layout: albedo.r (8 bits) | albedo.g (8 bits) | albedo.b (8 bits) | unused (8 bits)
} DNcompressedVoxel;

//a chunk of voxels, voxels are stored this way to save memory and accelerate ray casting
typedef struct DNchunk
{
	DNivec3 pos;         //the chunk's position within the entire map
	GLuint used;         //whether the chunk is currently in use (chunks can be empty)
	GLuint updated;      //whether the chunk has updates not yet pushed to the GPU
	GLuint numVoxels;    //the number of filled voxels this chunk contains, used to identify empty chunks for removal
	GLuint numVoxelsGpu; //the number of voxels this chunk stores on the GPU

	DNcompressedVoxel voxels[8][8][8]; //the grid of voxels in this chunk, of size DN_CHUNK_SIZE
} DNchunk;

//a handle to a chunk, along with some meta-data
typedef struct DNchunkHandle
{
	unsigned char flag;      //0 = does not exist, 1 = loaded on CPU, in the future, may be used for streaming from disk
	unsigned int chunkIndex; //the index at which the chunk's data can be found, invalid if flag = 0
} DNchunkHandle;

//represents a group of voxels on the GPU
typedef struct DNvoxelNode
{
	unsigned int size; //the node's size, in DNvoxels
	size_t startPos;   //the node's start position, in DNvoxels
	DNivec3 chunkPos;  //the position of the chunk that owns the node, if invalid, the node is unused
} DNvoxelNode;

//material properties for a voxel
typedef struct DNmaterial
{
	DNvec3 padding;		//for gpu alignment

	GLuint emissive;    //whether or not the voxel emits light, represented as a uint for GPU memory alignment
	GLfloat opacity;    //the voxel's opacity, in the range [0.0, 1.0]
	GLfloat specular;   //the percent of light reflected specularly by a voxel in the range [0.0, 1.0]
	GLuint reflectType; //only for materials where specular > 0.0; 0 = do not reflect the sky color, 1 = reflect the sky color, 2 = specular highlight only (much faster to calculate)
	GLuint shininess;   //only for materials where specular > 0.0; determines how perfect the reflcetions are, the greater this number, the closer to a perfect mirror
} DNmaterial;

//a voxel volume, both on the CPU and the GPU
typedef struct DNvolume
{	
	//opengl handles:
	GLuint glMapBufferID;   		  //READ ONLY | The openGL buffer ID for the map buffer on the GPU
	GLuint glChunkBufferID;			  //READ ONLY | The openGL buffer ID for the chunk buffer on the GPU
	GLuint glVoxelBufferID;			  //READ ONLY | The openGL buffer ID for the voxel buffer on the GPU

	//data parameters:
	DNuvec3 mapSize; 	    		  //READ ONLY | The size, in DNchunks, of the map
	size_t chunkCap;                  //READ ONLY | The current number of DNchunks that are stored CPU-side by this map. The length of chunks
	size_t chunkCapGPU;               //READ ONLY | The current number of DNchunks that are stored GPU-side bu this map.
	size_t nextChunk;                 //READ ONLY | The next known empty chunk index. Used to speed up adding new chunks
	size_t voxelCap;				  //READ ONLY | The current number of DNvoxels that are stored GPU-side by this map
	size_t numVoxelNodes;             //READ ONLY | The current number of nodes that the GPU voxel data is broken up into
	size_t numLightingRequests;       //READ ONLY | The number of chunks queued to have their lighting updated
	size_t lightingRequestCap;        //READ ONLY | The maximum number of chunks that can be stored in lightingRequests

	//data:
	DNchunkHandle* map; 		 	  //READ-WRITE | The map of chunks. An array with length = mapSize.x * mapSize.y * mapSize.z
	DNchunk* chunks; 	 		 	  //READ-WRITE | The array of chunks that the volume has
	DNmaterial* materials;			  //READ-WRITE | The array of materials that the volume has
	GLuint* lightingRequests;         //READ-WRITE | An array of chunk indices (represented as a uvec4 due to a need for aligment on the gpu, only the x component is used), signifies which chunks will have their lighting updated when DN_update_lighting() is called
	DNivec3* gpuChunkLayout;		  //READ ONLY  | An array representing the chunk layout on the GPU. Represents the position in the map that each chunk is.
	DNvoxelNode* gpuVoxelLayout;      //READ ONLY  | An array representing the voxel layout on the GPU

	//camera parameters:
	DNvec3 camPos; 			  		  //READ-WRITE | The camera's position relative to this map, in DNchunks
	DNvec3 camOrient; 		  		  //READ-WRITE | The camera's orientation, in degrees. Expressed as {pitch, yaw, roll}
	float camFOV; 			 		  //READ-WRITE | The camera's field of view, measured in degrees
	unsigned int camViewMode; 		  //READ-WRITE | The camera's view mode, 0 = normal; 1 = albedo only; 2 = diffuse light only; 3 = specular light only; 4 = normals

	//lighting parameters:
	DNvec3 sunDir; 					  //READ-WRITE | The direction pointing towards the sun
	DNvec3 sunStrength; 			  //READ-WRITE | The strength of sunlight, also determines the color
	DNvec3 ambientLightStrength; 	  //READ-WRITE | The minimum lighting for every voxel
	unsigned int diffuseBounceLimit;  //READ-WRITE | The maximum number of bounces for diffuse rays, can greatly affect performance
	unsigned int specBounceLimit; 	  //READ-WRITE | The maximum number of bounces for specular rays, can greatly affect performance
	float shadowSoftness; 			  //READ-WRITE | How soft shadows from direct light appear

	//sky parameters:
	bool useCubemap;				  //READ-WRITE | Whether or not the volume should sample a cubemap for the sky color, otherwise a gradient will be used
	unsigned int glCubemapTex;		  //READ-WRITE | The openGL texture handle to the cubemap to be sampled from, this MUST be set to a valid handle if useCubemap is true
	DNvec3 skyGradientBot;			  //READ-WRITE | If the volume does NOT sample a cubemap, the color at the bottom of the gradient for the sky color
	DNvec3 skyGradientTop;			  //READ-WRITE | If the volume does NOT sample a cubemap, the color at the top of the gradient for the sky color

	unsigned int frameNum;			  //READ ONLY  | Used to split the lighting calculations over multiple frames, determines the current frame. In the range [0, lightingSplit - 1]
	float lastTime;					  //READ ONLY  | Used to ensure that each group of chunks receives the same time value, even when they are calculated at different times
} DNvolume;

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

/*Creates a new DNvolume with the specified parameters 
 * @param mapSize the size, in DNchunks, of the map
 * @param textureSize the size, in pixels, of the texture that is rendered to
 * @param minChunks determines the minimum number of chunks that will be loaded on the GPU. If set too low, the volume may lag for the first few frames. 
 * @returns the new map or NULL if the volume creation failed in any way
 */
DNvolume* DN_create_volume(DNuvec3 mapSize, unsigned int minChunks);
/* Deletes a DNvolume, should be called whenever a map is no longer needed to avoid memory leaks
 * @param vol the volume to delete
 */
void DN_delete_volume(DNvolume* vol);

/* Loads a DNvolume from a file
 * @param filePath the path to the file to load from
 * @param textureSize the size, in pixels, of the texture that is rendered to
 * @param minChunks determines the minimum number of chunks that will be loaded on the GPU. If set too low, the volume may lag for the first few frames. 
 * @returns the loaded map or NULL, on failure
 */
DNvolume* DN_load_volume(const char* filePath, unsigned int minChunks);
/* Saves a DNvolume to a file
 * @param filePath the path of the file to save to
 * @param vol the volume to save
 * @returns true on success, false on failure
 */
bool DN_save_volume(const char* filePath, DNvolume* vol);

//--------------------------------------------------------------------------------------------------------------------------------//
//DRAWING:

/* Calculates the view and projection matrices for the current camera position.
 * @param vol the volume to render
 * @param aspectRatio the aspect ratio of the target texture (height / width)
 * @param nearPlane the camera's near clipping plane, used for composing with rasterized objects
 * @param farPlane the camera's far clipping plane, used for composing with rasterized objects
 * @param view populated with the camera's view matrix, use this when rendering rasterized objects
 * @param projection populated with the camera's projection matrix, use this when rendering rasterized objects
 */
void DN_set_view_projection_matrices(DNvolume* vol, float aspectRatio, float nearPlane, float farPlane, DNmat4* view, DNmat4* projection);

/* Draws the voxel map to the texture
 * @param vol the volume to render
 * @param outputTexture the texture that will be drawn to, must have an internal format of GL_RGBA32F. The textures resolution will be used to determine how many work groups are dispatched
 * @param view the camera's view matrix
 * @param projection the camera's projection matrix
 * @param rasterColorTexture the handle to the color buffer for rasterized objects, or -1 if not composing with rasterized objects
 * @param rasterDepthTexture the handle to the depth buffer for rasterized objects, or -1 if not composing with rasterized objects
 */
void DN_draw(DNvolume* vol, unsigned int outputTexture, DNmat4 view, DNmat4 projection, int rasterColorTexture, int rasterDepthTexture);

/* Updates the lighting on every chunk currently in a map's lightingRequests
 * @param vol the volume to update
 * @param numDiffuseSamples the number of diffuse lighting samples to take
 * @param maxDiffuseSamples the maximum number of diffuse samples that a chunk can store at once. The lower the value, the faster lighting can change but the more flickering that can occur. 1000 is a good base value
 * @param time the current running time of the application, used for random seeding
 */
void DN_update_lighting(DNvolume* vol, unsigned int numDiffuseSamples, unsigned int maxDiffuseSamples, float time);

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY:

/* Allocates space for and creates a new chunk CPU-side. It should never be necessary to call as it is called automatically. NOTE: does NOT do any bounds checking
 * @param vol the volume to add a new chunk to
 * @param pos the position within the map to add the chunk at
 * @returns the index to the newly created chunk
 */
unsigned int DN_add_chunk(DNvolume* vol, DNivec3 pos);
/* Removes and frees space for a chunk. It should never be necessary to call as it is called automatically. NOTE: does NOT do any bounds checking
 * @param vol the volume to remove the chunk from
 * @param pos the position to remove the chunk from
 */
void DN_remove_chunk(DNvolume* vol, DNivec3 pos);

/* Updates the gpu-side data for a map, this should be called every frame (or every few frames)
 * @param vol the volume to sync
 * @param op the operation to perform on the volume. DN_READ will only query the gpu for visible chunks. DN_WRITE will upload voxel data to the GPU if updated or requested. DN_READ_WRITE will do both
 * @param lightingSplit the number of frames to split the lighting calculation over. For example, if this is set to 5 only 1/5 of the chunks will have their lighting updated each frame, increasing performace
 */
void DN_sync_gpu(DNvolume* vol, DNmemOp op, unsigned int lightingSplit);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP SETTINGS:

/* Sets a map's size
 * @param vol the volume to change
 * @param size the new size, in DNchunks
 * @returns true on success, false on failure
 */
bool DN_set_map_size(DNvolume* vol, DNuvec3 size);

/* Sets a map's maximum number of chunks. It should never be necessary to call as it is called automatically
 * @param vol the volume to change
 * @param num the new maximum number of chunks
 * @returns true on success, false on failure
 */
bool DN_set_max_chunks(DNvolume* vol, unsigned int num);
/* Sets a map's maximum number of chunks in VRAM. It should never be necessary to call as it is called automatically
 * @param vol the volume to change
 * @param num the new maximum number of chunks
 * @returns true on success, false on failure
 */
bool DN_set_max_chunks_gpu(DNvolume* vol, size_t num);
/* Set's a map's maximum bumber of voxels in VRAM. It should never be necessary to call as it is called automatically
 * @param vol the volume to change
 * @param num the new maximum number of chunks
 * @returns true on success, false on failure
 */
bool DN_set_max_voxels_gpu(DNvolume* vol, size_t num);

/* Sets the current maximum number of lighting updates the volume can request at once. It should never be necessary to call as it is called automatically
 * @param vol the volume to change
 * @param num the new maximum number of lighting requests
 * @returns true on success, false on failure
 */
bool DN_set_max_lighting_requests(DNvolume* vol, unsigned int num);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAP UTILITY:

/* Determines whether or not a given map position is inside the map bounds
 * @param vol the volume to check
 * @param pos the position within the map to check, measured in DNchunks
 * @returns true if pos is in the range [0, map->mapSize - 1], false if not
 */
bool DN_in_map_bounds(DNvolume* vol, DNivec3 pos);
/* Determines whether or not a given chunk position is inside the chunk bounds
 * @param pos the position within a chunk to check, measured in DNvoxels
 * @returns true if pos is in the range [0, DN_CHUNK_SIZE - 1], false if not
 */
bool DN_in_chunk_bounds(DNivec3 pos);

/* Gets a voxel from the volume. NOTE: does NOT do any bounds checking and does NOT check if the requested chunk even exists
 * @param vol the volume to get the voxel from
 * @param volPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @returns the voxel
 */
DNvoxel DN_get_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos);
/* Gets a compressed voxel from the volume. NOTE: does NOT do any bounds checking and does NOT check if the requested chunk even exists
 * @param vol the volume to get the voxel from
 * @param volPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @returns the voxel
 */
DNcompressedVoxel DN_get_compressed_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos);

/* Sets a voxel in the volume. Automatically allocates memory for new chunks if needed. NOTE: does NOT do any bounds checking
 * @param vol the volume to set the voxel in
 * @param volPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @param voxel the voxel to set
 */
void DN_set_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos, DNvoxel voxel);
/* Sets a voxel in the volume. Automatically allocates memory for new chunks if needed. NOTE: does NOT do any bounds checking
 * @param vol the volume to set the voxel in
 * @param volPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @param voxel the voxel to set
 */
void DN_set_compressed_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos, DNcompressedVoxel voxel);
/* Removes a voxel from the volume. Automatically frees memory if needed. NOTE: does NOT do any bounds checking
 * @param vol the volume to set the voxel in
 * @param volPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 */
void DN_remove_voxel(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos);

/* Determines if a chunk exists (not empty and loaded). NOTE: does NOT do any bounds checking
 * @param vol the volume to check
 * @param pos the position within the map, in DNchunks, to check
 * @returns true if the chunk exists and is loaded, false otherwise
 */
bool DN_does_chunk_exist(DNvolume* vol, DNivec3 pos);
/* Determines if a voxel exists (material != 255). NOTE: does NOT do any bounds checking
 * @param vol the volume to check
 * @param volPos the voxel's chunk's position within the map, measured in DNchunks
 * @param chunkPos the voxel's position within the chunk, measured in DNvoxels
 * @returns true if the voxel is not empty (material != 255), false otherwise
 */
bool DN_does_voxel_exist(DNvolume* vol, DNivec3 mapPos, DNivec3 chunkPos);

/* Casts a ray into the volume and determines the voxel that was hit, if any
 * @param vol the volume to cast the ray into
 * @param rayDir the ray's direction
 * @param rayPos the ray's origin position
 * @param maxSteps the maximum number of steps to allow the ray to take
 * @param hitPos if a voxel is hit, populated with its position within the map, measured in DNvoxels
 * @param hitVoxel if a voxel is hit, populated with the voxel that was hit
 * @param hitNormal if a voxel is hit, populated with the normal along which the voxel was hit
 * @returns true if a voxel was hit, false otherwise
 */
bool DN_step_map(DNvolume* vol, DNvec3 rayDir, DNvec3 rayPos, unsigned int maxSteps, DNivec3* hitPos, DNvoxel* hitVoxel, DNivec3* hitNormal);

//--------------------------------------------------------------------------------------------------------------------------------//
//GENERAL UTILITY:

/* Separates a voxel's position into a map and a chunk position
 * @param pos the position to separate, measured in DNvoxels
 * @param volPos populated with the voxel's chunk's position within the map, equivalent to pos / DN_CHUNK_SIZE
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
