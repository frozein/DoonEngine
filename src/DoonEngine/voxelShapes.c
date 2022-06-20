#include "voxelShapes.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/quaternion.h"
#include "utility/shader.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPE PARAMETERS:

static DNvec3 center;
static float radius;
static DNvec3 length;

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPE SDFs:

static float _DN_sdf_box(DNvec3 p)
{
	DNvec3 q = DN_vec3_subtract((DNvec3){fabs(p.x), fabs(p.y), fabs(p.z)}, length);

	float q1 = DN_vec3_length((DNvec3){max(q.x, 0.0), max(q.y, 0.0), max(q.z, 0.0)});
	float q2 = min(max(max(q.x, q.y), q.z), 0.0);

	return q1 + q2;
}

//--------------------------------------------------------------------------------------------------------------------------------//
//UTILITY:

//transforms the bounding box of a shape
static void _DN_transform_bounding_box(DNvec3* min, DNvec3* max, DNmat4 transform)
{
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
		DNvec4 point = {points[i].x, points[i].y, points[i].z, 1.0f};
		point = DN_mat4_mult_vec4(transform, point);

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
	const float h = 0.0001;

	DNvec4 xP = DN_mat4_mult_vec4(invTransform, DN_vec4_add(p, (DNvec4){h, 0, 0, 0}));
	DNvec4 yP = DN_mat4_mult_vec4(invTransform, DN_vec4_add(p, (DNvec4){0, h, 0, 0}));
	DNvec4 zP = DN_mat4_mult_vec4(invTransform, DN_vec4_add(p, (DNvec4){0, 0, h, 0}));

	float x = sdf((DNvec3){xP.x, xP.y, xP.z}) - dist;
	float y = sdf((DNvec3){yP.x, yP.y, yP.z}) - dist;
	float z = sdf((DNvec3){zP.x, zP.y, zP.z}) - dist;

	float maxNormal = max(max(fabs(x), fabs(y)), fabs(z));
	return DN_vec3_scale((DNvec3){x, y, z}, 1 / maxNormal);
}

//@param min the minimum point on the bounding box of the UN-TRANSFORMED shape
static void _DN_shape(DNmap* map, DNmaterialHandle material, DNvec3 min, DNvec3 max, DNmat4 transform, float (*sdf)(DNvec3))
{
	DNmat4 invTransform = DN_mat4_inv(transform);

	_DN_transform_bounding_box(&min, &max, transform);
	DNivec3 iMin = {floorf(min.x), floorf(min.y), floorf(min.z)};
	DNivec3 iMax = {ceilf (max.x), ceilf (max.y), ceilf (max.z)};

	for(int z = iMin.z; z <= iMax.z; z++)
	{
		if(z / DN_CHUNK_SIZE.z >= map->mapSize.z || z < 0)
			continue;

		for(int y = iMin.y; y <= iMax.y; y++)
		{
			if(y / DN_CHUNK_SIZE.y >= map->mapSize.y || y < 0)
				continue;

			for(int x = iMin.x; x <= iMax.x; x++)
			{
				if(x / DN_CHUNK_SIZE.x >= map->mapSize.x || x < 0)
					continue;

				DNvec4 pos = {x, y, z, 1.0f};
				pos = DN_mat4_mult_vec4(invTransform, pos);
				float dist = sdf((DNvec3){pos.x, pos.y, pos.z});

				if(dist < 0.0)
				{
					DNivec3 mapPos;
					DNivec3 chunkPos;
					DN_separate_position((DNivec3){x, y, z}, &mapPos, &chunkPos);

					DNchunkHandle mapTile = map->map[DN_FLATTEN_INDEX(mapPos, map->mapSize)];
					if(mapTile.flag == 0 || !DN_does_voxel_exist(map, mapPos, chunkPos))
					{
						DNvoxel vox;
						vox.material = material;
						vox.normal = _DN_calc_normal((DNvec4){x, y, z, 1.0f}, invTransform, dist, sdf);
						DN_set_voxel(map, mapPos, chunkPos, vox);
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------------------//
//SHAPES

//this function is implemented specially to be more efficient
void DN_shapes_sphere(DNmap* map, DNmaterialHandle material, DNvec3 c, float r)
{
	DNivec3 min = {(int)floorf(c.x - r), (int)floorf(c.y - r), (int)floorf(c.z - r)};
	DNivec3 max = {(int)ceilf (c.x + r), (int)ceilf (c.y + r), (int)ceilf (c.z + r)};

	const float rSqr = r * r;
	for(int z = min.z; z <= max.z; z++)
	{
		if(z / DN_CHUNK_SIZE.z >= map->mapSize.z || z < 0)
			continue;

		for(int y = min.y; y <= max.y; y++)
		{
			if(y / DN_CHUNK_SIZE.y >= map->mapSize.y || y < 0)
				continue;

			for(int x = min.x; x <= max.x; x++)
			{
				if(x / DN_CHUNK_SIZE.x >= map->mapSize.x || x < 0)
					continue;

				float distSqr = (c.x - x) * (c.x - x) + (c.y - y) * (c.y - y) + (c.z - z) * (c.z - z);
				if(distSqr <= rSqr)
				{
					DNivec3 mapPos;
					DNivec3 chunkPos;
					DN_separate_position((DNivec3){x, y, z}, &mapPos, &chunkPos);

					DNchunkHandle mapTile = map->map[DN_FLATTEN_INDEX(mapPos, map->mapSize)];
					if(mapTile.flag == 0 || !DN_does_voxel_exist(map, mapPos, chunkPos))
					{
						DNvoxel vox;
						vox.material = material;
						vox.normal = (DNvec3){x - c.x, y - c.y, z - c.z};

						float maxNormal = fabs(vox.normal.x);
						if(fabs(vox.normal.y) > maxNormal)
							maxNormal = fabs(vox.normal.y);
						if(fabs(vox.normal.z) > maxNormal)
							maxNormal = fabs(vox.normal.z);

						vox.normal = DN_vec3_scale(vox.normal, 1.0f / maxNormal);

						DN_set_voxel(map, mapPos, chunkPos, vox);
					}
				}
			}
		}
	}
}

void DN_shapes_box(DNmap* map, DNmaterialHandle material, DNvec3 c, DNvec3 len, DNvec3 orient)
{
	DNmat4 transform;
	transform = DN_mat4_translate(DN_MAT4_IDENTITY, c);
	transform = DN_mat4_rotate_euler(transform, orient);

	length = len;

	_DN_shape(map, material, DN_vec3_scale(len, -1.0f), len, transform, _DN_sdf_box);
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

static int _DN_read_int( FILE *fp ) 
{
	int v = 0;
	fread( &v, 4, 1, fp );
	return v;
}

void _DN_read_chunk_info( FILE *fp, VoxFileChunk* chunk ) 
{
	chunk->id       = _DN_read_int(fp);
	chunk->len      = _DN_read_int(fp);
	chunk->childLen = _DN_read_int(fp);
	chunk->endPtr   = ftell(fp) + chunk->len + chunk->childLen;
}

bool DN_load_vox_file(const char* path, DNvoxelModel* model)
{
	FILE* fp = fopen(path, "rb");

	if(fp == NULL)
	{
		DN_ERROR_LOG("ERROR - UNABLE TO OPEN FILE \"%s\"\n", path);
		return false;
	}

	unsigned int numVoxels;
	VoxFileVoxel* tempVoxels = NULL;
	VoxFileVoxel palette[256];
	model->voxels = NULL;

	//check if actually is vox file:
	int idNum = _DN_read_int(fp);
	if(idNum != CHUNK_ID('V', 'O', 'X', ' '))
	{
		DN_ERROR_LOG("ERROR - NOT A VALID .VOX FILE\n");
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
		case CHUNK_ID('S', 'I', 'Z', 'E'): //SIZE chunk
		{
			//read size:
			DNuvec3 size;
			size.x = _DN_read_int(fp);
			size.y = _DN_read_int(fp);
			size.z = _DN_read_int(fp);
			model->size = size;

			break;
		}
		case CHUNK_ID('X', 'Y', 'Z', 'I'):
		{
			if(tempVoxels != NULL)
				DN_FREE(tempVoxels);

			numVoxels = _DN_read_int(fp);

			tempVoxels = DN_MALLOC(numVoxels * sizeof(VoxFileVoxel));
			fread(tempVoxels, sizeof(VoxFileVoxel), numVoxels, fp);

			break;
		}
		case CHUNK_ID('R', 'G', 'B', 'A'):
		{
			fread(&palette[1], sizeof(VoxFileVoxel), 255, fp);
			break;
		}
		}

		fseek(fp, chunk.endPtr, SEEK_SET); //skip to end of chunk
	}

	size_t modelSize = model->size.x * model->size.y * model->size.z;
	model->voxels = DN_MALLOC(modelSize * sizeof(DNcompressedVoxel));
	for(int i = 0; i < modelSize; i++)
		model->voxels[i].normal = UINT32_MAX;

	for(int i = 0; i < numVoxels; i++)
	{
		DNvoxel vox;
		VoxFileVoxel pos = tempVoxels[i];
		//TODO: add functionality for sharing the palette with magicavoxel or something

		vox.material = pos.w;
		vox.normal = (DNvec3){0.0f, 1.0f, 0.0f};

		DNivec3 pos2 = {pos.x, pos.z, pos.y};
		model->voxels[DN_FLATTEN_INDEX(pos2, model->size)] = DN_compress_voxel(vox); //have to invert z and y because magicavoxel has z as the up axis
	}

	return true;
}

void DN_free_model(DNvoxelModel model)
{
	DN_FREE(model.voxels);
}

void DN_calculate_model_normals(unsigned int r, DNvoxelModel* model)
{
	for(int xC = 0; xC < model->size.x; xC++)
	for(int yC = 0; yC < model->size.y; yC++)
	for(int zC = 0; zC < model->size.z; zC++)
	{
		DNvec3 sum = {0.0f, 0.0f, 0.0f};

		DNivec3 center = {xC, yC, zC};
		int iC = DN_FLATTEN_INDEX(center, model->size);
		DNvoxel voxC = DN_decompress_voxel(model->voxels[iC]);
		if(voxC.material == 255)
			continue;

		for(int xP = xC - (int)r; xP <= xC + (int)r; xP++)
		for(int yP = yC - (int)r; yP <= yC + (int)r; yP++)
		for(int zP = zC - (int)r; zP <= zC + (int)r; zP++)
		{
			if(xP < 0 || yP < 0 || zP < 0)
				continue;
			
			if(xP >= model->size.x || yP >= model->size.y || zP >= model->size.z)
				continue;

			DNivec3 pos = {xP, yP, zP};
			int iP = DN_FLATTEN_INDEX(pos, model->size);

			if(iP == iC)
				continue;

			if(DN_decompress_voxel(model->voxels[iP]).material < 255)
			{
				DNvec3 toCenter = {xP - xC, yP - yC, zP - zC};
				float dist = DN_vec3_dot(toCenter, toCenter);
				dist *= dist; //reduces artifacts at the cost of some smoothness
				toCenter = DN_vec3_scale(toCenter, 1.0f / dist);
				sum = DN_vec3_add(sum, toCenter);
			}
		}

		if(sum.x == 0.0f && sum.y == 0.0f && sum.z == 0.0f)
			sum = (DNvec3){0.0f, 1.0f, 0.0f};

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

void DN_place_model_into_world(DNmap* map, DNvoxelModel model, DNivec3 pos)
{
	for(int x = 0; x < model.size.x; x++)
	for(int y = 0; y < model.size.y; y++)
	for(int z = 0; z < model.size.z; z++)
	{
		DNivec3 curPos = {x, y, z};
		int iModel = DN_FLATTEN_INDEX(curPos, model.size);
		if(DN_decompress_voxel(model.voxels[iModel]).material < 255)
		{
			DNivec3 worldPos = {pos.x + x, pos.y + y, pos.z + z};
			DNivec3 chunkPos = {worldPos.x / DN_CHUNK_SIZE.x, worldPos.y / DN_CHUNK_SIZE.y, worldPos.z / DN_CHUNK_SIZE.z};

			if(DN_in_map_bounds(map, chunkPos))
			{
				DNivec3 localPos = {worldPos.x % DN_CHUNK_SIZE.x, worldPos.y % DN_CHUNK_SIZE.y, worldPos.z % DN_CHUNK_SIZE.z};
				DN_set_compressed_voxel(map, chunkPos, localPos, model.voxels[iModel]);
			}
		}
	}
}