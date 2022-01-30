#include "render.h"
#include "math/vector.h"
#include "memory.h"
#include "shader.h"

#include <GLAD/glad.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

//--------------------------------------------------------------------------------------------------------------------------------//

unsigned int gen_vertex_object(unsigned int size, void* vertexData, DrawType drawType)
{
	GLenum drawTypeGL;

	switch(drawType)
	{
	case DRAW_STATIC:
		drawTypeGL = GL_STATIC_DRAW;
		break;
	case DRAW_DYNAMIC:
		drawTypeGL = GL_DYNAMIC_DRAW;
		break;
	case DRAW_STREAM:
		drawTypeGL = GL_STREAM_DRAW;
		break;
	}

	//generate:
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	//bind:
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//set data:
	glBufferData(GL_ARRAY_BUFFER, size, vertexData, drawTypeGL);

	return VAO;
}

unsigned int gen_vertex_object_indices(unsigned int size, void* vertexData, unsigned int indexSize, unsigned int* indices, DrawType drawType)
{
	GLenum drawTypeGL;

	switch(drawType)
	{
	case DRAW_STATIC:
		drawTypeGL = GL_STATIC_DRAW;
		break;
	case DRAW_DYNAMIC:
		drawTypeGL = GL_DYNAMIC_DRAW;
		break;
	case DRAW_STREAM:
		drawTypeGL = GL_STREAM_DRAW;
		break;
	}

	//generate:
	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	//bind:
	glBindVertexArray(VAO);

	//set vertex data:
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, size, vertexData, drawTypeGL);

	//set index data:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, indices, drawTypeGL);

	return VAO;
}

void free_vertex_object(unsigned int vertexObj)
{
	int numAttrib = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &numAttrib);
	glBindVertexArray(vertexObj);

	for(int i = 0; i < numAttrib; i++)
	{
		int VBO = 0;
		glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &VBO);
		if(VBO > 0)
		{
			glDeleteBuffers(1, &VBO);
		}
	}

	glDeleteVertexArrays(1, &vertexObj);
}

void set_vertex_attribute(unsigned int vertexObj, unsigned int layout, unsigned int numVals, DrawType type, unsigned int stride, unsigned int offset)
{
	glBindVertexArray(vertexObj);

	glVertexAttribPointer(layout, numVals, type, GL_FALSE, stride, (void*)(long long)offset);
	glEnableVertexAttribArray(layout);
}

void draw_vertex_object(unsigned int vertexObj, unsigned int count, unsigned int startPos)
{
	glBindVertexArray(vertexObj);
	glDrawArrays(GL_TRIANGLES, startPos, count);
}

void draw_vertex_object_indices(unsigned int vertexObj, unsigned int count, unsigned int startPos)
{
	glBindVertexArray(vertexObj);
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(startPos * sizeof(unsigned int)));
}