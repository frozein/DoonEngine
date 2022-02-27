#ifndef DN_MATH_COMMON_H
#define DN_MATH_COMMON_H
#include <GLAD/glad.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//STRUCTS:

//A 2-dimensional vector
typedef struct vec2 {
	GLfloat x, y;
} vec2;

//A 3-dimensional vector
typedef struct vec3 {
	GLfloat x, y, z;
} vec3;

//A 4-dimensional vector
typedef struct vec4 {
	GLfloat x, y, z, w;
} vec4;

//-----------------------------//
//NOTE: no math functions exist for integer vectors

//A 2-dimensional integer vector
typedef struct ivec2
{
	GLint x, y;
} ivec2;

//A 3-dimensional integer vector
typedef struct ivec3
{
	GLint x, y, z;
} ivec3;

//A 4-dimensional integer vector
typedef struct ivec4
{
	GLint x, y, z, w;
} ivec4;

//-----------------------------//
//NOTE: no math functions exist for integer vectors

//A 2-dimensional integer vector
typedef struct uvec2
{
	GLuint x, y;
} uvec2;

//A 3-dimensional integer vector
typedef struct uvec3
{
	GLuint x, y, z;
} uvec3;

//A 4-dimensional integer vector
typedef struct uvec4
{
	GLuint x, y, z, w;
} uvec4;

//-----------------------------//

//A 2x2 matrix
typedef struct mat2 {
	GLfloat m[2][2];
} mat2;

//A 3x3 matrix
typedef struct mat3 {
	GLfloat m[3][3];
} mat3;

//A 4x4 matrix
typedef struct mat4 {
	GLfloat m[4][4];
} mat4;

//-----------------------------//

typedef struct quat {
	GLfloat x, y, z, w;
} quat;

//--------------------------------------------------------------------------------------------------------------------------------//
//CONSTANTS:

#define PI             3.14159265359f
#define PI_SQUARED     9.86960440109f
#define INV_PI         0.31830988618f
#define INV_PI_SQUARED 0.10132118364f

#define DEG_TO_RAD 0.01745329251f
#define RAD_TO_DEG 57.2957795131f

//--------------------------------------------------------------------------------------------------------------------------------//
//MATRIX INITIALIZERS:

#define MAT2_IDENTITY (mat2){{{1.0f, 0.0f}, \
					          {0.0f, 1.0f}}}
#define MAT2_ZERO     (mat2){{{0.0f, 0.0f}, \
					          {0.0f, 0.0f}}}

#define MAT3_IDENTITY (mat3){{{1.0f, 0.0f, 0.0f}, \
					          {0.0f, 1.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 1.0f}}}
#define MAT3_ZERO     (mat3){{{0.0f, 0.0f, 0.0f}, \
					          {0.0f, 0.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 0.0f}}}

#define MAT4_IDENTITY (mat4){{{1.0f, 0.0f, 0.0f, 0.0f}, \
					          {0.0f, 1.0f, 0.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 1.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 0.0f, 1.0f}}}
#define MAT4_ZERO     (mat4){{{0.0f, 0.0f, 0.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 0.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 0.0f, 0.0f}, \
					    	  {0.0f, 0.0f, 0.0f, 0.0f}}}

#endif