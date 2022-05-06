#ifndef DN_VECTOR_H
#define DN_VECTOR_H

#include "common.h"

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC2:

//Prints a vector2 to the console
void vec2_print(vec2 v);

//Returns the sum of 2 vector2s
vec2 vec2_add(vec2 v1, vec2 v2);
//Returns the difference of 2 vector2s
vec2 vec2_subtract(vec2 v1, vec2 v2);
//Returns the result of a component-wise multiplication of 2 vector2s
vec2 vec2_mult(vec2 v1, vec2 v2);
//Returns the the result of scaling a vector2 by a scalar value
vec2 vec2_scale(vec2 v, GLfloat s);

//Returns a normalized vector2
vec2 vec2_normalize(vec2 v);
//Clamps each value of the vector2 between min and max and returns the result
vec2 vec2_clamp(vec2 v, GLfloat min, GLfloat max);
//Returns the result of a linear interpolation between 2 vector2s
vec2 vec2_lerp(vec2 from, vec2 to, GLfloat a);

//Returns the length/magnitude of a vector2
GLfloat vec2_length(vec2 v);
//Returns the dot product of 2 vector2s
GLfloat vec2_dot(vec2 v1, vec2 v2);
//Returns the distance between 2 vector2s
GLfloat vec2_distance(vec2 v1, vec2 v2);

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC3:

//Prints a vector3 to the console
void vec3_print(vec3 v);

//Returns the sum of 2 vector3s
vec3 vec3_add(vec3 v1, vec3 v2);
//Returns the difference of 2 vector3s
vec3 vec3_subtract(vec3 v1, vec3 v2);
//Returns the result of a component-wise multiplication of 2 vector3s
vec3 vec3_mult(vec3 v1, vec3 v2);
//Returns the the result of scaling a vector3 by a scalar value
vec3 vec3_scale(vec3 v, GLfloat s);

//Returns a normalized vector3
vec3 vec3_normalize(vec3 v);
//Clamps each value of the vector3 between min and max and returns the result
vec3 vec3_clamp(vec3 v, GLfloat min, GLfloat max);
//Returns the result of a linear interpolation between 2 vector3s
vec3 vec3_lerp(vec3 from, vec3 to, GLfloat a);
//Returns the cross product of 2 vector3s
vec3 vec3_cross(vec3 v1, vec3 v2);

//Returns the length/magnitude of a vector3
GLfloat vec3_length(vec3 v);
//Returns the dot product of 2 vector3s
GLfloat vec3_dot(vec3 v1, vec3 v2);
//Returns the distance between 2 vector3s
GLfloat vec3_distance(vec3 v1, vec3 v2);

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC4:

//Prints a vector4 to the console
void vec4_print(vec4 v);

//Returns the sum of 2 vector4s
vec4 vec4_add(vec4 v1, vec4 v2);
//Returns the difference of 2 vector4s
vec4 vec4_subtract(vec4 v1, vec4 v2);
//Returns the result of a component-wise multiplication of 2 vector4s
vec4 vec4_mult(vec4 v1, vec4 v2);
//Returns the the result of scaling a vector4 by a scalar value
vec4 vec4_scale(vec4 v, GLfloat s);

//Returns a normalized vector4
vec4 vec4_normalize(vec4 v);
//Clamps each value of the vector4 between min and max and returns the result
vec4 vec4_clamp(vec4 v, GLfloat min, GLfloat max);
//Returns the result of a linear interpolation between 2 vector4s
vec4 vec4_lerp(vec4 from, vec4 to, GLfloat a);

//Returns the length/magnitude of a vector4
GLfloat vec4_length(vec4 v);
//Returns the dot product of 2 vector4s
GLfloat vec4_dot(vec4 v1, vec4 v2);
//Returns the distance between 2 vector4s
GLfloat vec4_distance(vec4 v1, vec4 v2);

#endif