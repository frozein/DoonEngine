#include "voxelShapes.h"
#include "math/vector.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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

bool DN_load_vox_file(const char* path, DNvoxelModel* model, DNmaterialHandle material)
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
	model->voxels = DN_MALLOC(modelSize * sizeof(DNvoxelGPU));
	for(int i = 0; i < modelSize; i++)
		model->voxels[i].albedo = UINT32_MAX;

	for(int i = 0; i < numVoxels; i++)
	{
		DNvoxel vox;
		VoxFileVoxel pos = tempVoxels[i];
		VoxFileVoxel color = palette[pos.w];

		vox.albedo.x = pow((float)color.x / 255.0f, 2.2f);
		vox.albedo.y = pow((float)color.y / 255.0f, 2.2f);
		vox.albedo.z = pow((float)color.z / 255.0f, 2.2f);
		vox.material = material;
		vox.normal = (DNvec3){0.0f, 1.0f, 0.0f};

		DNivec3 pos2 = {pos.x, pos.z, pos.y};
		model->voxels[DN_FLATTEN_INDEX(pos2, model->size)] = DN_voxel_to_voxelGPU(vox); //have to invert z and y because magicavoxel has z as the up axis
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
		DNvoxel voxC = DN_voxelGPU_to_voxel(model->voxels[iC]);
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

			if(DN_voxelGPU_to_voxel(model->voxels[iP]).material < 255)
			{
				DNvec3 toCenter = {xP - xC, yP - yC, zP - zC};
				float dist = vec3_dot(toCenter, toCenter);
				toCenter = vec3_scale(toCenter, 1.0f / dist);
				sum = vec3_add(sum, toCenter);
			}
		}

		if(sum.x == 0.0f && sum.y == 0.0f && sum.z == 0.0f)
			sum = (DNvec3){0.0f, 1.0f, 0.0f};

		float maxNormal = fabs(sum.x);
		if(fabs(sum.y) > maxNormal)
			maxNormal = fabs(sum.y);
		if(fabs(sum.z) > maxNormal)
			maxNormal = fabs(sum.z);

		sum = vec3_scale(sum, -1.0f / maxNormal);
		voxC.normal = sum;
		model->voxels[iC] = DN_voxel_to_voxelGPU(voxC);
	}
}

void DN_place_model_into_world(DNvoxelModel model, DNivec3 pos)
{
	for(int x = 0; x < model.size.x; x++)
	for(int y = 0; y < model.size.y; y++)
	for(int z = 0; z < model.size.z; z++)
	{
		DNivec3 curPos = {x, y, z};
		int iModel = DN_FLATTEN_INDEX(curPos, model.size);
		if(DN_voxelGPU_to_voxel(model.voxels[iModel]).material < 255)
		{
			DNivec3 worldPos = {pos.x + x, pos.y + y, pos.z + z};
			DNivec3 chunkPos = {worldPos.x / DN_CHUNK_SIZE.x, worldPos.y / DN_CHUNK_SIZE.y, worldPos.z / DN_CHUNK_SIZE.z};

			if(DN_in_voxel_map_bounds(chunkPos))
			{
				int iWorld = DN_FLATTEN_INDEX(chunkPos, DN_voxel_map_size());
				dnVoxelMap[iWorld].flag = 1;
				dnVoxelChunks[iWorld].voxels[worldPos.x % DN_CHUNK_SIZE.x][worldPos.y % DN_CHUNK_SIZE.y][worldPos.z % DN_CHUNK_SIZE.z] = model.voxels[iModel];
			}
		}
	}
}
