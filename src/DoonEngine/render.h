#ifndef DN_RENDER_H
#define DN_RENDER_H

#include "math/common.h"
#include <stdbool.h>

//--------------------------------------------------------------------------------------------------------------------------------//

//Determines where the vertex data will be in memory
typedef enum DrawType
{
	DRAW_STATIC, //Use if drawn often and set infrequently
	DRAW_DYNAMIC, //Use if drawn infrequently and set infrequently
	DRAW_STREAM //Use if drawn often and set often
} DrawType;

//--------------------------------------------------------------------------------------------------------------------------------//

//Generates a vertex object
unsigned int gen_vertex_object(unsigned int size, void* vertexData, DrawType drawType);
//Generates a vertex object with an index list
unsigned int gen_vertex_object_indices(unsigned int size, void* vertexData, unsigned int indexSize, unsigned int* indices, DrawType drawType);

//Sets a vertex attribute
void set_vertex_attribute(unsigned int vertexObj, unsigned int layout, unsigned int numVals, DrawType type, unsigned int stride, unsigned int offset);

//Draws a vertex object
void draw_vertex_object(unsigned int vertexObj, unsigned int count, unsigned int startPos);
//Draws a vertex object with index list. ONLY USE IF YOUR OBJECT WAS GENERATED WITH AN INDEX LIST
void draw_vertex_object_indices(unsigned int vertexObj, unsigned int count, unsigned int startPos);
//Frees all of the buffers attached to a vertex object. DO NOT USE IF THERE ARE SHARED BUFFERS OR A BUFFER IS BOUND TO MULTIPLE ATTRIBUTES
void free_vertex_object(unsigned int vertexObj);

#endif