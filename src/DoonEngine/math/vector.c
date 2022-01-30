#include "vector.h"
#include <stdio.h>
#include <math.h>

#define NORMALIZE_TOLERANCE 0.0001

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC2:

void vec2_print(vec2 v)
{
	printf("%f, ", v.x);
	printf("%f\n", v.y);
}

vec2 vec2_add(vec2 v1, vec2 v2)
{
	return (vec2){v1.x + v2.x, v1.y + v2.y};
}

vec2 vec2_subtract(vec2 v1, vec2 v2)
{
	return (vec2){v1.x - v2.x, v1.y - v2.y};
}

vec2 vec2_mult(vec2 v1, vec2 v2)
{
	return (vec2){v1.x * v2.x, v1.y * v2.y};
}

vec2 vec2_scale(vec2 v, GLfloat s)
{
	return (vec2){v.x * s, v.y * s};
}

vec2 vec2_normalize(vec2 v)
{
	vec2 res = v;

	GLfloat mag = vec2_length(v);
	if(fabsf(mag - 1.0f) >= NORMALIZE_TOLERANCE && mag != 0.0f)
	{
		mag = 1.0f / sqrtf(mag);
		res.x *= mag;
		res.y *= mag;
	}

	return res;
}

vec2 vec2_lerp(vec2 from, vec2 to, GLfloat a)
{
	GLfloat a2 = 1.0f - a;

	return (vec2){from.x * a2 + to.x * a, from.y * a2 + to.y * a};
} 

GLfloat vec2_length(vec2 v)
{
	return v.x * v.x + v.y * v.y;
}

GLfloat vec2_dot(vec2 v1, vec2 v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

GLfloat vec2_distance(vec2 v1, vec2 v2)
{
	GLfloat x = v1.x - v2.x;
	GLfloat y = v1.y - v2.y;

	return sqrtf(x * x + y * y);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC3:

void vec3_print(vec3 v)
{
	printf("%f, ", v.x);
	printf("%f, ", v.y);
	printf("%f\n", v.z);
}

vec3 vec3_add(vec3 v1, vec3 v2)
{
	return (vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

vec3 vec3_subtract(vec3 v1, vec3 v2)
{
	return (vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

vec3 vec3_mult(vec3 v1, vec3 v2)
{
	return (vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

vec3 vec3_scale(vec3 v, GLfloat s)
{
	return (vec3){v.x * s, v.y * s, v.z * s};
}

vec3 vec3_normalize(vec3 v)
{
	vec3 res = v;

	GLfloat mag = vec3_length(v);
	if(fabsf(mag - 1.0f) >= NORMALIZE_TOLERANCE && mag != 0.0f)
	{
		mag = 1.0f / sqrtf(mag);
		res.x *= mag;
		res.y *= mag;
		res.z *= mag;
	}

	return res;
}

vec3 vec3_lerp(vec3 from, vec3 to, GLfloat a)
{
	GLfloat a2 = 1.0f - a;

	return (vec3){from.x * a2 + to.x * a, from.y * a2 + to.y * a, from.z * a2 + to.z * a};
} 

vec3 vec3_cross(vec3 v1, vec3 v2)
{
	return (vec3){v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

GLfloat vec3_length(vec3 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

GLfloat vec3_dot(vec3 v1, vec3 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

GLfloat vec3_distance(vec3 v1, vec3 v2)
{
	GLfloat x = v1.x - v2.x;
	GLfloat y = v1.y - v2.y;
	GLfloat z = v1.z - v2.z;

	return sqrtf(x * x + y * y + z * z);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC4:

void vec4_print(vec4 v)
{
	printf("%f, ", v.x);
	printf("%f, ", v.y);
	printf("%f, ", v.z);
	printf("%f\n", v.w);
}

vec4 vec4_add(vec4 v1, vec4 v2)
{
	return (vec4){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

vec4 vec4_subtract(vec4 v1, vec4 v2)
{
	return (vec4){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

vec4 vec4_mult(vec4 v1, vec4 v2)
{
	return (vec4){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

vec4 vec4_scale(vec4 v, GLfloat s)
{
	return (vec4){v.x * s, v.y * s, v.z * s, v.w * s};
}

vec4 vec4_normalize(vec4 v)
{
	vec4 res = v;

	GLfloat mag = vec4_length(v);
	if(fabsf(mag - 1.0f) >= NORMALIZE_TOLERANCE && mag != 0.0f)
	{
		mag = 1.0f / sqrtf(mag);
		res.x *= mag;
		res.y *= mag;
		res.z *= mag;
		res.w *= mag;
	}

	return res;
}

vec4 vec4_lerp(vec4 from, vec4 to, GLfloat a)
{
	GLfloat a2 = 1.0f - a;

	return (vec4){from.x * a2 + to.x * a, from.y * a2 + to.y * a, from.z * a2 + to.z * a, from.w * a2 + to.w * a};
}

GLfloat vec4_length(vec4 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

GLfloat vec4_dot(vec4 v1, vec4 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

GLfloat vec4_distance(vec4 v1, vec4 v2)
{
	GLfloat x = v1.x - v2.x;
	GLfloat y = v1.y - v2.y;
	GLfloat z = v1.z - v2.z;
	GLfloat w = v1.w - v2.w;

	return sqrtf(x * x + y * y + z * z + w * w);
}