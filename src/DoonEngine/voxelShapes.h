#ifndef DN_VOXEL_SHAPES_H
#define DN_VOXEL_SHAPES_H

#include "globals.h"
#include "voxel.h"
#include "math/common.h"

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPES:

//NOTE: the shapes are generated using distance fields and thus may not appear exactly as specified when scaled down to the low-resolution voxel map
//for example, a cylinder with a height of 5 might only be 4 voxels tall due to rounding

//Places a sphere into the map at the given center and with the given radius
void DN_shape_sphere(DNmap* map, DNmaterialHandle material, DNvec3 c, float r);
//Places a box into the map with the given parameters @param c the center @param len the distance from the center to the edge of the box
void DN_shape_box(DNmap* map, DNmaterialHandle material, DNvec3 c, DNvec3 len, DNvec3 orient);
//Places a box into the map with the given parameters @param c the center @param len the distance from the center to the edge of the box @param r the radius of the corners
void DN_shape_rounded_box(DNmap* map, DNmaterialHandle material, DNvec3 c, DNvec3 len, float r, DNvec3 orient);
//Places a torus into the map with the given parameters @param c the center @param minR the interior radius @param maxR the exterior radius
void DN_shape_torus(DNmap* map, DNmaterialHandle material, DNvec3 c, float ra, float rb, DNvec3 orient);
//Places an ellipsoid into the map with the given parameters @c the center @param r the lengths of the semi-axis of the ellipsoid
void DN_shape_ellipsoid(DNmap* map, DNmaterialHandle material, DNvec3 c, DNvec3 r, DNvec3 orient);
//Places a cylinder into the map with the given parameters @param c the center @param r the radius @param h the height
void DN_shape_cylinder(DNmap* map, DNmaterialHandle material, DNvec3 c, float r, float h, DNvec3 orient);
//Places a cone into the map with the given parameters @param b the base point of the cone @param r the radius @param h the height
void DN_shape_cone(DNmap* map, DNmaterialHandle material, DNvec3 b, float r, float h, DNvec3 orient);

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