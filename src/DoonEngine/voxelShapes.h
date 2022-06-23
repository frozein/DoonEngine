#ifndef DN_VOXEL_SHAPES_H
#define DN_VOXEL_SHAPES_H

#include "globals.h"
#include "voxel.h"
#include "math/common.h"

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPES:

//NOTE: the shapes are generated using distance fields and thus may not appear exactly as specified when scaled down to the low-resolution voxel map
//for example, a cylinder with a height of 5 might only be 4 voxels tall due to rounding

/* Places a sphere into a map
 * @param map the map to edit
 * @param material the material to use
 * @param c the sphere's center
 * @param r the sphere's radius
 */
void DN_shape_sphere(DNmap* map, uint8_t material, DNvec3 c, float r);
/* Places a box into a map
 * @param map the map to edit
 * @param material the material to use
 * @param c the box's center
 * @param len the distance from the center to the edge of the box, in each direction
 * @param orient the box's orientation, expressed as {pitch, yaw, roll}
 */
void DN_shape_box(DNmap* map, uint8_t material, DNvec3 c, DNvec3 len, DNvec3 orient);
/* Places a rounded box into a map
 * @param map the map to edit
 * @param material the material to use
 * @param c the box's center
 * @param len the distance from the center to the edge of the box, in each direction
 * @param r the box's radius
 * @param orient the box's orientation, expressed as {pitch, yaw, roll}
 */
void DN_shape_rounded_box(DNmap* map, uint8_t material, DNvec3 c, DNvec3 len, float r, DNvec3 orient);
/* Places a torus into a map
 * @param map the map to edit
 * @param material the material to use
 * @param c the torus's center
 * @param ra the center radius of the torus
 * @param rb the radius of the "ring" of the torus
 * @param orient the torus's orientation, expressed as {pitch, yaw, roll}
 */
void DN_shape_torus(DNmap* map, uint8_t material, DNvec3 c, float ra, float rb, DNvec3 orient);
/* Places an ellipsoid into a map
 * @param map the map to edit
 * @param material the material to use
 * @param c the center of the ellipsoid
 * @param r the lengths of the semi-axes of the ellipsoid
 * @param orient the ellipsoid's orientation, expressed as {pitch, yaw, roll}
 */
void DN_shape_ellipsoid(DNmap* map, uint8_t material, DNvec3 c, DNvec3 r, DNvec3 orient);
/* Places a cylinder into a map
 * @param map the map to edit
 * @param material the material to use
 * @param c the center of the cylinder
 * @param r the radius of the cylinder
 * @param h the height of the cylinder
 * @param orient the cylinder's orientation, expressed as {pitch, yaw, roll}
 */
void DN_shape_cylinder(DNmap* map, uint8_t material, DNvec3 c, float r, float h, DNvec3 orient);
/* Places a cone into a map
 * @param map the map to edit
 * @param material the material to use
 * @param b the position of the cone's base
 * @param r the radius of the cone
 * @param h the height of the cone
 * @param orient the cone's orientation, expressed as {pitch, yaw, roll}
 */
void DN_shape_cone(DNmap* map, uint8_t material, DNvec3 b, float r, float h, DNvec3 orient);

//--------------------------------------------------------------------------------------------------------------------------------//
//VOX FILE MODELS:

//a voxel model
typedef struct DNvoxelModel
{
	DNuvec3 size;
	DNcompressedVoxel* voxels;
} DNvoxelModel;

/* Loads a magicavoxel model from a .vox file
 * @param path the path to the file to load from
 * @param model populated with the model once loaded
 * @returns true on success, false on failure
 */
bool DN_load_vox_file(const char* path, uint8_t material, DNvoxelModel* model);
/* Frees the memory of a model
 * @param model the model to free
*/
void DN_free_model(DNvoxelModel model);

/* Calculates the normals for every voxel in a model as models loaded from magicavoxel have none
 * @param radius the radius, in DNvoxels, to use for the calculation. The larger this value, the smoother the normals but the longer the calculation time
 * @param model the model to calculate the normals for
 */
void DN_calculate_model_normals(unsigned int radius, DNvoxelModel* model);

/* Places a model into a map
 * @param map the map to place the model into
 * @param model the model to place
 * @param pos the position to place the model at, measured in DNvoxels. This is the minimum coordinate that the model will touch
 */
void DN_place_model_into_map(DNmap* map, DNvoxelModel model, DNivec3 pos);

#endif