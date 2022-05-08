#ifndef DN_MATH_COMMON_H
#define DN_MATH_COMMON_H
#include <GLAD/glad.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//STRUCTS:

//A 2-dimensional vector
typedef struct DNvec2 {
	GLfloat x, y;
} DNvec2;

//A 3-dimensional vector
typedef struct DNvec3 {
	GLfloat x, y, z;
} DNvec3;

//A 4-dimensional vector
typedef struct DNvec4 {
	GLfloat x, y, z, w;
} DNvec4;

//-----------------------------//
//NOTE: no math functions exist for integer vectors

//A 2-dimensional integer vector
typedef struct DNivec2
{
	GLint x, y;
} DNivec2;

//A 3-dimensional integer vector
typedef struct DNivec3
{
	GLint x, y, z;
} DNivec3;

//A 4-dimensional integer vector
typedef struct DNivec4
{
	GLint x, y, z, w;
} DNivec4;

//-----------------------------//
//NOTE: no math functions exist for integer vectors

//A 2-dimensional integer vector
typedef struct DNuvec2
{
	GLuint x, y;
} DNuvec2;

//A 3-dimensional integer vector
typedef struct DNuvec3
{
	GLuint x, y, z;
} DNuvec3;

//A 4-dimensional integer vector
typedef struct DNuvec4
{
	GLuint x, y, z, w;
} DNuvec4;

//-----------------------------//

//A 2x2 matrix
typedef struct DNmat2 {
	GLfloat m[2][2];
} DNmat2;

//A 3x3 matrix
typedef struct DNmat3 {
	GLfloat m[3][3];
} DNmat3;

//A 4x4 matrix
typedef struct DNmat4 {
	GLfloat m[4][4];
} DNmat4;

//-----------------------------//

typedef struct DNquat {
	GLfloat x, y, z, w;
} DNquat;

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

#define DN_MAT2_IDENTITY (DNmat2){{{1.0f, 0.0f}, \
					         	   {0.0f, 1.0f}}}
#define DN_MAT2_ZERO     (DNmat2){{{0.0f, 0.0f}, \
					          	   {0.0f, 0.0f}}}

#define DN_MAT3_IDENTITY (DNmat3){{{1.0f, 0.0f, 0.0f}, \
					               {0.0f, 1.0f, 0.0f}, \
					    	  	   {0.0f, 0.0f, 1.0f}}}
#define DN_MAT3_ZERO     (DNmat3){{{0.0f, 0.0f, 0.0f}, \
					          	   {0.0f, 0.0f, 0.0f}, \
					    	 	   {0.0f, 0.0f, 0.0f}}}

#define DN_MAT4_IDENTITY (DNmat4){{{1.0f, 0.0f, 0.0f, 0.0f}, \
					          	   {0.0f, 1.0f, 0.0f, 0.0f}, \
					    	 	   {0.0f, 0.0f, 1.0f, 0.0f}, \
					    	 	   {0.0f, 0.0f, 0.0f, 1.0f}}}
#define DN_MAT4_ZERO     (DNmat4){{{0.0f, 0.0f, 0.0f, 0.0f}, \
					    	 	   {0.0f, 0.0f, 0.0f, 0.0f}, \
					    		   {0.0f, 0.0f, 0.0f, 0.0f}, \
					    		   {0.0f, 0.0f, 0.0f, 0.0f}}}

#endif