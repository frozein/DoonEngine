#ifndef DN_VOXEL_SHAPES_H
#define DN_VOXEL_SHAPES_H

#include "globals.h"
#include "voxel.h"
#include "math/common.h"

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPES:

//--------------------------------------------------------------------------------------------------------------------------------//
//SERIALIZATION:



//--------------------------------------------------------------------------------------------------------------------------------//
//VOX FILE MODELS:

//a voxel model, not placed into the world
typedef struct DNvoxelModel
{
	DNuvec3 size;
	DNcompressedVoxel* voxels;
} DNvoxelModel;

//Loads a model from a magicaVoxel file (.vox). Returns true on success, false on failure
bool DN_load_vox_file(const char* path, DNvoxelModel* model);
//Frees the memory for a model
void DN_free_model(DNvoxelModel model);

//Calculates the normals for every voxel in a model, the larger the radius, the smoother the normals but the longer the calculation time
void DN_calculate_model_normals(unsigned int radius, DNvoxelModel* model);

//Places a model into the world
void DN_place_model_into_world(DNmap* map, DNvoxelModel model, DNivec3 pos);

#endif