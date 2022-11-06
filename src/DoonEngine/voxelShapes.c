#include "voxelShapes.h"
#include "utility/shader.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPE PARAMETERS:

static float radius;
static float radiusB;
static float height;
static DNvec2 angles;
static DNvec3 radii;
static DNvec3 length;

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPE SDFs:

//ALL SDF FUNCTIONS FROM https://iquilezles.org/articles/distfunctions/

static float _DN_sdf_box(DNvec3 p)
{
	DNvec3 q = DN_vec3_sub((DNvec3){fabs(p.x), fabs(p.y), fabs(p.z)}, length);

	float q1 = DN_vec3_length(DN_vec3_max(q, (DNvec3){0.0f, 0.0f, 0.0f}));
	float q2 = fmin(fmax(fmax(q.x, q.y), q.z), 0.0);

	return q1 + q2;
}

static float _DN_sdf_rounded_box(DNvec3 p)
{
	return _DN_sdf_box(p) - radius;
}

static float _DN_sdf_torus(DNvec3 p)
{
	DNvec2 q = {DN_vec2_length((DNvec2){p.x, p.z}) - radius, p.y};
	return (DN_vec2_length(q) - radiusB);
}

static float _DN_sdf_ellipsoid(DNvec3 p)
{
	return (DN_vec3_length(DN_vec3_div(p, radii)) - 1.0f) * fmin(fmin(radii.x, radii.y), radii.z);
}

static float _DN_sdf_cylinder(DNvec3 p)
{
	DNvec2 d = {DN_vec2_length((DNvec2){p.x, p.z}) - radius, fabs(p.y) - height};
	return fmin(fmax(d.x, d.y), 0.0) + DN_vec2_length(DN_vec2_max(d, (DNvec2){0.0f, 0.0f}));
}

static float _DN_sdf_cone(DNvec3 p)
{
	float q = DN_vec2_length((DNvec2){p.x, p.z});
	return fmax(DN_vec2_dot((DNvec2){angles.x, angles.y}, (DNvec2){q, p.y}), -height - p.y);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//UTILITY:

//transforms the bounding box of a shape
static void _DN_transform_bounding_box(DNvec3* min, DNvec3* max, DNmat4 transform)
{
	//get all vertices of the bounding box:
	DNvec3 points[8];
	points[0] = *min;
	points[1] = (DNvec3){min->x, min->y, max->z};
	points[2] = (DNvec3){min->x, max->y, min->z};
	points[3] = (DNvec3){min->x, max->y, max->z};
	points[4] = (DNvec3){max->x, min->y, min->z};
	points[5] = (DNvec3){max->x, min->y, max->z};
	points[6] = (DNvec3){max->x, max->y, min->z};
	points[7] = *max;

	*min = (DNvec3){ INFINITY,  INFINITY,  INFINITY};
	*max = (DNvec3){-INFINITY, -INFINITY, -INFINITY};
	for(int i = 0; i < 8; i++)
	{
		//transform the vertex:
		DNvec4 point = {points[i].x, points[i].y, points[i].z, 1.0f};
		point = DN_mat4_mult_vec4(transform, point);

		//check to see if it is a max/min:
		if(point.x > max->x)
			max->x = point.x;
		if(point.x < min->x)
			min->x = point.x;

		if(point.y > max->y)
			max->y = point.y;
		if(point.y < min->y)
			min->y = point.y;

		if(point.z > max->z)
			max->z = point.z;
		if(point.z < min->z)
			min->z = point.z;
	}
}

//calculates the normal of a point on an sdf
static DNvec3 _DN_calc_normal(DNvec4 p, DNmat4 invTransform, float dist, float (*sdf)(DNvec3))
{
	const float h = 0.01;

	DNvec4 xP = DN_mat4_mult_vec4(invTransform, DN_vec4_add(p, (DNvec4){h, 0, 0, 0}));
	DNvec4 yP = DN_mat4_mult_vec4(invTransform, DN_vec4_add(p, (DNvec4){0, h, 0, 0}));
	DNvec4 zP = DN_mat4_mult_vec4(invTransform, DN_vec4_add(p, (DNvec4){0, 0, h, 0}));

	float x = sdf((DNvec3){xP.x, xP.y, xP.z}) - dist;
	float y = sdf((DNvec3){yP.x, yP.y, yP.z}) - dist;
	float z = sdf((DNvec3){zP.x, zP.y, zP.z}) - dist;

	float maxNormal = fmax(fmax(fabs(x), fabs(y)), fabs(z));
	return DN_vec3_scale((DNvec3){x, y, z}, 1 / maxNormal);
}

//places a shape in a volume, using the given SDF
static void _DN_shape(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 min, DNvec3 max, DNmat4 transform, float (*sdf)(DNvec3))
{
	DNmat4 invTransform = DN_mat4_inv(transform);

	//find min and max points:
	_DN_transform_bounding_box(&min, &max, transform);
	DNivec3 iMin = {(int)floorf(min.x), (int)floorf(min.y), (int)floorf(min.z)};
	DNivec3 iMax = {(int)ceilf (max.x), (int)ceilf (max.y), (int)ceilf (max.z)};

	//go 1 more pixel in each direction if removing so that normals can be set:
	if (voxel.material == DN_MATERIAL_EMPTY)
	{
		iMin = (DNivec3){iMin.x - 1, iMin.y - 1, iMin.z - 1};
		iMax = (DNivec3){iMax.x + 1, iMax.y + 1, iMax.z + 1};
	}

	//separate positions:
	DNivec3 mapMin, mapMax, chunkMin, chunkMax;
	DN_separate_position(iMin, &mapMin, &chunkMin);
	DN_separate_position(iMax, &mapMax, &chunkMax);

	//bounds check:
	mapMin.x = QM_MAX(mapMin.x, 0);
	mapMin.y = QM_MAX(mapMin.y, 0);
	mapMin.z = QM_MAX(mapMin.z, 0);
	mapMax.x = QM_MIN(mapMax.x, vol->mapSize.x - 1);
	mapMax.y = QM_MIN(mapMax.y, vol->mapSize.y - 1);
	mapMax.z = QM_MIN(mapMax.z, vol->mapSize.z - 1);

	//loop over every chunk:
	for(int mZ = mapMin.z; mZ <= mapMax.z; mZ++)
	for(int mY = mapMin.y; mY <= mapMax.y; mY++)
	for(int mX = mapMin.x; mX <= mapMax.x; mX++)
	{
		DNivec3 mapPos = {mX, mY, mZ};
		DNchunkHandle mapTile = vol->map[DN_FLATTEN_INDEX(mapPos, vol->mapSize)];

		//loop over every voxel in chunk:
		for(int cZ = 0; cZ < DN_CHUNK_SIZE; cZ++)
		for(int cY = 0; cY < DN_CHUNK_SIZE; cY++)
		for(int cX = 0; cX < DN_CHUNK_SIZE; cX++)
		{
			DNivec3 chunkPos = {cX, cY, cZ};

			//calculate the sdf distance:
			DNvec4 pos = {mX * DN_CHUNK_SIZE + cX, mY * DN_CHUNK_SIZE + cY, mZ * DN_CHUNK_SIZE + cZ, 1.0f};
			DNvec4 transformedPos = DN_mat4_mult_vec4(invTransform, pos);
			float dist = sdf((DNvec3){transformedPos.x, transformedPos.y, transformedPos.z});

			if(dist < 0.0)
			{
				if(mapTile.flag == 0 || (!DN_does_voxel_exist(vol, mapPos, chunkPos) || voxel.material == DN_MATERIAL_EMPTY))
				{
					DNvoxel finalVox = voxel;
					if(voxel.material != DN_MATERIAL_EMPTY)
					{
						DNvec3 normal = _DN_calc_normal(pos, invTransform, dist, sdf);
						
						if(func != NULL)
							finalVox = func((DNvec3){pos.x, pos.y, pos.z}, normal, voxel, min, max, invTransform);
						else
							finalVox.normal = normal;
					}
					DN_set_voxel(vol, mapPos, chunkPos, finalVox);
				}
			}
			else if(dist < 1.0f && voxel.material == DN_MATERIAL_EMPTY)
			{
				if(mapTile.flag != 0 && DN_does_voxel_exist(vol, mapPos, chunkPos))
				{
					DNvoxel oldVox = DN_get_voxel(vol, mapPos, chunkPos);
					oldVox.normal = DN_vec3_scale(_DN_calc_normal(pos, invTransform, dist, sdf), -1.0f);
					DN_set_voxel(vol, mapPos, chunkPos, oldVox);
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPES

//note: this function is implemented differently so as to be more efficient
void DN_shape_sphere(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 c, float r)
{
	//find min and max points:
	DNvec3 min = {c.x - r, c.y - r, c.z - r};
	DNvec3 max = {c.x + r, c.y + r, c.z + r};

	DNivec3 iMin = {(int)floorf(min.x), (int)floorf(min.y), (int)floorf(min.z)};
	DNivec3 iMax = {(int)ceilf (max.x), (int)ceilf (max.y), (int)ceilf (max.z)};

	//go 1 more pixel in each direction if removing so that normals can be set:
	if (voxel.material == DN_MATERIAL_EMPTY)
	{
		iMin = (DNivec3){iMin.x - 1, iMin.y - 1, iMin.z - 1};
		iMax = (DNivec3){iMax.x + 1, iMax.y + 1, iMax.z + 1};
	}

	//separate positions:
	DNivec3 mapMin, mapMax, chunkMin, chunkMax;
	DN_separate_position(iMin, &mapMin, &chunkMin);
	DN_separate_position(iMax, &mapMax, &chunkMax);

	//bounds check:
	mapMin.x = QM_MAX(mapMin.x, 0);
	mapMin.y = QM_MAX(mapMin.y, 0);
	mapMin.z = QM_MAX(mapMin.z, 0);
	mapMax.x = QM_MIN(mapMax.x, vol->mapSize.x);
	mapMax.y = QM_MIN(mapMax.y, vol->mapSize.y);
	mapMax.z = QM_MIN(mapMax.z, vol->mapSize.z);

	const float r2 = r * r;
	const float r12 = (r + 1.0f) * (r + 1.0f);

	//loop over every chunk:
	for(int mZ = mapMin.z; mZ <= mapMax.z; mZ++)
	for(int mY = mapMin.y; mY <= mapMax.y; mY++)
	for(int mX = mapMin.x; mX <= mapMax.x; mX++)
	{
		DNivec3 mapPos = {mX, mY, mZ};
		DNchunkHandle mapTile = vol->map[DN_FLATTEN_INDEX(mapPos, vol->mapSize)];

		//loop over every voxel in chunk:
		for(int cZ = 0; cZ < DN_CHUNK_SIZE; cZ++)
		for(int cY = 0; cY < DN_CHUNK_SIZE; cY++)
		for(int cX = 0; cX < DN_CHUNK_SIZE; cX++)
		{
			DNivec3 chunkPos = {cX, cY, cZ};
			DNvec3 pos = {mX * DN_CHUNK_SIZE + cX, mY * DN_CHUNK_SIZE + cY, mZ * DN_CHUNK_SIZE + cZ};
			DNvec3 fromCenter = DN_vec3_sub(pos, c);

			float dist2 = DN_vec3_dot(fromCenter, fromCenter); 
			if(dist2 < r2)
			{
				if(mapTile.flag == 0 || (!DN_does_voxel_exist(vol, mapPos, chunkPos) || voxel.material == DN_MATERIAL_EMPTY))
				{
					DNvoxel finalVox = voxel;
					if(voxel.material != DN_MATERIAL_EMPTY)
					{
						float maxNormal = fmax(fmax(fabs(fromCenter.x), fabs(fromCenter.y)), fabs(fromCenter.z));
						fromCenter = DN_vec3_scale(fromCenter, 1.0f / maxNormal);

						if(func != NULL)
							finalVox = func(pos, fromCenter, voxel, min, max, DN_mat4_identity());
						else
							finalVox.normal = fromCenter;
					}

					DN_set_voxel(vol, mapPos, chunkPos, finalVox);
				}
			}
			else if(voxel.material == DN_MATERIAL_EMPTY && dist2 < r12)
			{
				if(mapTile.flag != 0 && DN_does_voxel_exist(vol, mapPos, chunkPos))
				{
					DNvoxel oldVox = DN_get_voxel(vol, mapPos, chunkPos);
					float maxNormal = fmax(fmax(fabs(fromCenter.x), fabs(fromCenter.y)), fabs(fromCenter.z));
					fromCenter = DN_vec3_scale(fromCenter, -1.0f / maxNormal);

					if(func != NULL)
						oldVox = func(pos, fromCenter, oldVox, min, max, DN_mat4_identity());
					else
						oldVox.normal = fromCenter;

					DN_set_voxel(vol, mapPos, chunkPos, oldVox);				}
			}
		}
	}
}

void DN_shape_box(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 c, DNvec3 len, DNquaternion orient)
{
	DNmat4 transform;
	transform = DN_mat4_translate(c);
	transform = DN_mat4_mult(transform, DN_quaternion_to_mat4(orient));

	length = len;

	_DN_shape(vol, voxel, func, DN_vec3_scale(len, -1.0f), len, transform, _DN_sdf_box);
}

void DN_shape_rounded_box(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 c, DNvec3 len, float r, DNquaternion orient)
{
	DNmat4 transform;
	transform = DN_mat4_translate(c);
	transform = DN_mat4_mult(transform, DN_quaternion_to_mat4(orient));

	length = len;
	radius = r;

	DNvec3 min = {-len.x - r, -len.y - r, -len.z - r};
	DNvec3 max = { len.x + r,  len.x + r,  len.x + r};

	_DN_shape(vol, voxel, func, min, max, transform, _DN_sdf_rounded_box);
}

void DN_shape_torus(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 c, float ra, float rb, DNquaternion orient)
{
	DNmat4 transform;
	transform = DN_mat4_translate(c);
	transform = DN_mat4_mult(transform, DN_quaternion_to_mat4(orient));

	radius = ra;
	radiusB = rb;

	DNvec3 min = (DNvec3){-(ra + rb), -rb, -(ra + rb)};
	DNvec3 max = (DNvec3){ (ra + rb),  rb,  (ra + rb)};

	_DN_shape(vol, voxel, func, min, max, transform, _DN_sdf_torus);
}

void DN_shape_ellipsoid(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 c, DNvec3 r, DNquaternion orient)
{
	DNmat4 transform;
	transform = DN_mat4_translate(c);
	transform = DN_mat4_mult(transform, DN_quaternion_to_mat4(orient));

	radii = r;

	DNvec3 min = {-r.x, -r.y, -r.z};
	DNvec3 max = { r.x,  r.y,  r.z};

	_DN_shape(vol, voxel, func, min, max, transform, _DN_sdf_ellipsoid);
}

void DN_shape_cylinder(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 c, float r, float h, DNquaternion orient)
{
	DNmat4 transform;
	transform = DN_mat4_translate(c);
	transform = DN_mat4_mult(transform, DN_quaternion_to_mat4(orient));

	radius = r;
	height = h / 2;

	DNvec3 min = {-r, -h / 2, -r};
	DNvec3 max = { r,  h / 2,  r};

	_DN_shape(vol, voxel, func, min, max, transform, _DN_sdf_cylinder);
}

void DN_shape_cone(DNvolume* vol, DNvoxel voxel, VoxelTransformFunc func, DNvec3 b, float r, float h, DNquaternion orient)
{
	DNmat4 transform;
	b.y += h;
	transform = DN_mat4_translate(b);
	transform = DN_mat4_mult(transform, DN_quaternion_to_mat4(orient));

	float hyp = sqrtf(r * r + h * h);
	angles = (DNvec2){h / hyp, r / hyp};
	height = h;

	DNvec3 min = {-r, -h, -r};
	DNvec3 max = { r,  0,  r};

	_DN_shape(vol, voxel, func, min, max, transform, _DN_sdf_cone);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//MODEL UTILITIES:

typedef struct VoxFileVoxel
{
	unsigned char x, y, z, w;
} VoxFileVoxel;

typedef struct VoxFileChunk
{
	uint32_t id;
	uint32_t len;
	uint32_t childLen;
	size_t endPtr;
} VoxFileChunk;

#define CHUNK_ID(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

//reads a chunks info from a file
void _DN_read_chunk_info(FILE *fp, VoxFileChunk* chunk) 
{
	fread(&chunk->id, sizeof(int), 1, fp);
	fread(&chunk->len, sizeof(int), 1, fp);
	fread(&chunk->childLen, sizeof(int), 1, fp);
	chunk->endPtr   = ftell(fp) + chunk->len + chunk->childLen;
}

bool DN_load_vox_file(const char* path, int material, DNvoxelModel* model)
{
	FILE* fp = fopen(path, "rb"); 

	if(fp == NULL)
	{
		char message[256];
		sprintf(message, "failed to open file \"%s\" for reading", path);
		m_DN_message_callback(DN_MESSAGE_FILE_IO, DN_MESSAGE_ERROR, message);
		return false;
	}

	unsigned int numVoxels;
	VoxFileVoxel* tempVoxels = NULL;
	VoxFileVoxel palette[256];
	model->voxels = NULL;

	//check if actually is vox file:
	int idNum;
	fread(&idNum, sizeof(int), 1, fp);
	if(idNum != CHUNK_ID('V', 'O', 'X', ' '))
	{
		char message[256];
		sprintf(message, "file \"%s\" is not a valod .vox file", path);
		m_DN_message_callback(DN_MESSAGE_FILE_IO, DN_MESSAGE_ERROR, message);
		return false;
	}

	fseek( fp, 4, SEEK_CUR ); //skip version num

	VoxFileChunk mainChunk; //skip main chunk (never has content)
	_DN_read_chunk_info(fp, &mainChunk);

	//iterate remaining chunks:
	while(ftell(fp) < mainChunk.endPtr) 
	{
		VoxFileChunk chunk;
		_DN_read_chunk_info(fp, &chunk);

		switch(chunk.id)
		{
		case CHUNK_ID('S', 'I', 'Z', 'E'): //size chunk
		{
			//read size:
			DNuvec3 size;
			fread(&size.x, sizeof(int), 1, fp);
			fread(&size.y, sizeof(int), 1, fp);
			fread(&size.z, sizeof(int), 1, fp);
			model->size = size;

			break;
		}
		case CHUNK_ID('X', 'Y', 'Z', 'I'): //voxel data chunk
		{
			if(tempVoxels != NULL)
				DN_FREE(tempVoxels);

			fread(&numVoxels, sizeof(int), 1, fp);

			tempVoxels = DN_MALLOC(numVoxels * sizeof(VoxFileVoxel));
			fread(tempVoxels, sizeof(VoxFileVoxel), numVoxels, fp);

			break;
		}
		case CHUNK_ID('R', 'G', 'B', 'A'): //palette chunk
		{
			fread(&palette[1], sizeof(VoxFileVoxel), 255, fp);
			break;
		}
		}

		fseek(fp, chunk.endPtr, SEEK_SET); //skip to end of chunk
	}

	//allocate space for model:
	size_t modelSize = model->size.x * model->size.y * model->size.z;
	model->voxels = DN_MALLOC(modelSize * sizeof(DNcompressedVoxel));
	for(int i = 0; i < modelSize; i++)
		model->voxels[i].normal = UINT32_MAX;

	//set voxels:
	for(int i = 0; i < numVoxels; i++)
	{
		DNvoxel vox;
		VoxFileVoxel pos = tempVoxels[i];

		vox.material = material >= 0 ? material : pos.w;
		vox.normal = (DNvec3){0.0f, 1.0f, 0.0f};
		vox.albedo = (DNcolor){palette[pos.w].x, palette[pos.w].y, palette[pos.w].z};

		DNivec3 pos2 = {pos.x, pos.z, pos.y}; //have to invert z and y because magicavoxel has z as the up axis
		model->voxels[DN_FLATTEN_INDEX(pos2, model->size)] = DN_compress_voxel(vox);
	}

	return true;
}

void DN_free_model(DNvoxelModel model)
{
	DN_FREE(model.voxels);
}

void DN_calculate_model_normals(unsigned int r, DNvoxelModel* model)
{
	for(int zC = 0; zC < model->size.z; zC++)
	for(int yC = 0; yC < model->size.y; yC++)
	for(int xC = 0; xC < model->size.z; xC++)
	{
		DNvec3 sum = {0.0f, 0.0f, 0.0f}; //the sum of all the vectors pointing towards the center

		//get info about center voxel:
		DNivec3 center = {xC, yC, zC};
		int iC = DN_FLATTEN_INDEX(center, model->size);
		DNvoxel voxC = DN_decompress_voxel(model->voxels[iC]);
		if(voxC.material == DN_MATERIAL_EMPTY)
			continue;

		//loop over its neighbors:
		for(int zP = zC - (int)r; zP <= zC + (int)r; zP++)
		for(int yP = yC - (int)r; yP <= yC + (int)r; yP++)
		for(int xP = xC - (int)r; xP <= xC + (int)r; xP++)
		{
			if(xP < 0 || yP < 0 || zP < 0)
				continue;
			
			if(xP >= model->size.x || yP >= model->size.y || zP >= model->size.z)
				continue;

			DNivec3 pos = {xP, yP, zP};
			int iP = DN_FLATTEN_INDEX(pos, model->size);

			if(iP == iC)
				continue;

			if(DN_decompress_voxel(model->voxels[iP]).material != DN_MATERIAL_EMPTY)
			{
				//add vector to sum:
				DNvec3 toCenter = {xP - xC, yP - yC, zP - zC};
				float dist = DN_vec3_dot(toCenter, toCenter);
				dist *= dist; //reduces artifacts at the cost of some smoothness, not sure why
				toCenter = DN_vec3_scale(toCenter, 1.0f / dist);
				sum = DN_vec3_add(sum, toCenter);
			}
		}

		//TODO: maybe just check which faces are open in the case and pick the first one to avoid it pointing somewhere its not visible
		//if voxel has no neighbors or symmetrical neighbors, point it upward
		if(sum.x == 0.0f && sum.y == 0.0f && sum.z == 0.0f)
			sum = (DNvec3){0.0f, 1.0f, 0.0f};

		//scale normal so that it has a max component of 1:
		float maxNormal = fabs(sum.x);
		if(fabs(sum.y) > maxNormal)
			maxNormal = fabs(sum.y);
		if(fabs(sum.z) > maxNormal)
			maxNormal = fabs(sum.z);

		sum = DN_vec3_scale(sum, -1.0f / maxNormal);
		voxC.normal = sum;
		model->voxels[iC] = DN_compress_voxel(voxC);
	}
}

void DN_place_model_into_volume(DNvolume* vol, DNvoxelModel model, DNivec3 pos)
{
	for(int z = 0; z < model.size.z; z++)
	for(int y = 0; y < model.size.y; y++)
	for(int x = 0; x < model.size.x; x++)
	{
		DNivec3 curPos = {x, y, z};
		int iModel = DN_FLATTEN_INDEX(curPos, model.size);
		if(DN_decompress_voxel(model.voxels[iModel]).material != DN_MATERIAL_EMPTY)
		{
			DNivec3 worldPos = {pos.x + x, pos.y + y, pos.z + z};
			DNivec3 chunkPos = {worldPos.x / DN_CHUNK_SIZE, worldPos.y / DN_CHUNK_SIZE, worldPos.z / DN_CHUNK_SIZE};

			if(DN_in_map_bounds(vol, chunkPos))
			{
				DNivec3 localPos = {worldPos.x % DN_CHUNK_SIZE, worldPos.y % DN_CHUNK_SIZE, worldPos.z % DN_CHUNK_SIZE};
				DN_set_compressed_voxel(vol, chunkPos, localPos, model.voxels[iModel]);
			}
		}
	}
}
