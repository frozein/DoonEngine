#include "vector.h"
#include <stdio.h>
#include <math.h>

#define NORMALIZE_TOLERANCE 0.0001

//--------------------------------------------------------------------------------------------------------------------------------//
//UTILITY FUNCTIONS:

static GLfloat _DN_clamp(GLfloat v, GLfloat min, GLfloat max)
{
	if(v < min)
		return min;
	if(v > max)
		return max;
	
	return v;
}

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC2:

void DN_vec2_print(DNvec2 v)
{
	printf("%f, ", v.x);
	printf("%f\n", v.y);
}

DNvec2 DN_vec2_add(DNvec2 v1, DNvec2 v2)
{
	return (DNvec2){v1.x + v2.x, v1.y + v2.y};
}

DNvec2 DN_vec2_subtract(DNvec2 v1, DNvec2 v2)
{
	return (DNvec2){v1.x - v2.x, v1.y - v2.y};
}

DNvec2 DN_vec2_mult(DNvec2 v1, DNvec2 v2)
{
	return (DNvec2){v1.x * v2.x, v1.y * v2.y};
}

DNvec2 DN_vec2_divide(DNvec2 v1, DNvec2 v2)
{
	return (DNvec2){v1.x / v2.x, v1.y / v2.y};
}

DNvec2 DN_vec2_scale(DNvec2 v, GLfloat s)
{
	return (DNvec2){v.x * s, v.y * s};
}

DNvec2 DN_vec2_pow(DNvec2 v, GLfloat s)
{
	return (DNvec2){powf(v.x, s), powf(v.y, s)};
}

DNvec2 DN_vec2_normalize(DNvec2 v)
{
	DNvec2 res = v;

	GLfloat mag = v.x * v.x + v.y * v.y;
	if(fabsf(mag - 1.0f) >= NORMALIZE_TOLERANCE && mag != 0.0f)
	{
		mag = 1.0f / sqrtf(mag);
		res.x *= mag;
		res.y *= mag;
	}

	return res;
}

DNvec2 DN_vec2_clamp(DNvec2 v, GLfloat min, GLfloat max)
{
	return (DNvec2){_DN_clamp(v.x, min, max), _DN_clamp(v.y, min, max)};
}

DNvec2 DN_vec2_lerp(DNvec2 from, DNvec2 to, GLfloat a)
{
	GLfloat a2 = 1.0f - a;

	return (DNvec2){from.x * a2 + to.x * a, from.y * a2 + to.y * a};
} 

GLfloat DN_vec2_length(DNvec2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

GLfloat DN_vec2_dot(DNvec2 v1, DNvec2 v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

GLfloat DN_vec2_distance(DNvec2 v1, DNvec2 v2)
{
	GLfloat x = v1.x - v2.x;
	GLfloat y = v1.y - v2.y;

	return sqrtf(x * x + y * y);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC3:

void DN_vec3_print(DNvec3 v)
{
	printf("%f, ", v.x);
	printf("%f, ", v.y);
	printf("%f\n", v.z);
}

DNvec3 DN_vec3_add(DNvec3 v1, DNvec3 v2)
{
	return (DNvec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

DNvec3 DN_vec3_subtract(DNvec3 v1, DNvec3 v2)
{
	return (DNvec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

DNvec3 DN_vec3_mult(DNvec3 v1, DNvec3 v2)
{
	return (DNvec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

DNvec3 DN_vec3_divide(DNvec3 v1, DNvec3 v2)
{
	return (DNvec3){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
}

DNvec3 DN_vec3_scale(DNvec3 v, GLfloat s)
{
	return (DNvec3){v.x * s, v.y * s, v.z * s};
}

DNvec3 DN_vec3_pow(DNvec3 v, GLfloat s)
{
	return (DNvec3){powf(v.x, s), powf(v.y, s), powf(v.z, s)};
}

DNvec3 DN_vec3_normalize(DNvec3 v)
{
	DNvec3 res = v;

	GLfloat mag = v.x * v.x + v.y * v.y + v.z * v.z;
	if(fabsf(mag - 1.0f) >= NORMALIZE_TOLERANCE && mag != 0.0f)
	{
		mag = 1.0f / sqrtf(mag);
		res.x *= mag;
		res.y *= mag;
		res.z *= mag;
	}

	return res;
}

DNvec3 DN_vec3_clamp(DNvec3 v, GLfloat min, GLfloat max)
{
	return (DNvec3){_DN_clamp(v.x, min, max), _DN_clamp(v.y, min, max), _DN_clamp(v.z, min, max)};
}

DNvec3 DN_vec3_lerp(DNvec3 from, DNvec3 to, GLfloat a)
{
	GLfloat a2 = 1.0f - a;

	return (DNvec3){from.x * a2 + to.x * a, from.y * a2 + to.y * a, from.z * a2 + to.z * a};
} 

DNvec3 DN_vec3_cross(DNvec3 v1, DNvec3 v2)
{
	return (DNvec3){v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

GLfloat DN_vec3_length(DNvec3 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

GLfloat DN_vec3_dot(DNvec3 v1, DNvec3 v2)
{
	return sqrtf(v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

GLfloat DN_vec3_distance(DNvec3 v1, DNvec3 v2)
{
	GLfloat x = v1.x - v2.x;
	GLfloat y = v1.y - v2.y;
	GLfloat z = v1.z - v2.z;

	return sqrtf(x * x + y * y + z * z);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC4:

void DN_vec4_print(DNvec4 v)
{
	printf("%f, ", v.x);
	printf("%f, ", v.y);
	printf("%f, ", v.z);
	printf("%f\n", v.w);
}

DNvec4 DN_vec4_add(DNvec4 v1, DNvec4 v2)
{
	return (DNvec4){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

DNvec4 DN_vec4_subtract(DNvec4 v1, DNvec4 v2)
{
	return (DNvec4){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

DNvec4 DN_vec4_mult(DNvec4 v1, DNvec4 v2)
{
	return (DNvec4){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

DNvec4 DN_vec4_divide(DNvec4 v1, DNvec4 v2)
{
	return (DNvec4){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w};
}

DNvec4 DN_vec4_scale(DNvec4 v, GLfloat s)
{
	return (DNvec4){v.x * s, v.y * s, v.z * s, v.w * s};
}

DNvec4 DN_vec4_pow(DNvec4 v, GLfloat s)
{
	return (DNvec4){powf(v.x, s), powf(v.y, s), powf(v.z, s), powf(v.w, s)};
}

DNvec4 DN_vec4_normalize(DNvec4 v)
{
	DNvec4 res = v;

	GLfloat mag = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
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

DNvec4 DN_vec4_clamp(DNvec4 v, GLfloat min, GLfloat max)
{
	return (DNvec4){_DN_clamp(v.x, min, max), _DN_clamp(v.y, min, max), _DN_clamp(v.z, min, max), _DN_clamp(v.w, min, max)};
}

DNvec4 DN_vec4_lerp(DNvec4 from, DNvec4 to, GLfloat a)
{
	GLfloat a2 = 1.0f - a;

	return (DNvec4){from.x * a2 + to.x * a, from.y * a2 + to.y * a, from.z * a2 + to.z * a, from.w * a2 + to.w * a};
}

GLfloat DN_vec4_length(DNvec4 v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

GLfloat DN_vec4_dot(DNvec4 v1, DNvec4 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

GLfloat DN_vec4_distance(DNvec4 v1, DNvec4 v2)
{
	GLfloat x = v1.x - v2.x;
	GLfloat y = v1.y - v2.y;
	GLfloat z = v1.z - v2.z;
	GLfloat w = v1.w - v2.w;

	return sqrtf(x * x + y * y + z * z + w * w);
}