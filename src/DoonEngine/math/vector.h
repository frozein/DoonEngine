#ifndef DN_VECTOR_H
#define DN_VECTOR_H

#include "common.h"

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC2:

//Prints a vector2 to the console
void DN_vec2_print(DNvec2 v);

//Returns the sum of 2 vector2s
DNvec2 DN_vec2_add(DNvec2 v1, DNvec2 v2);
//Returns the difference of 2 vector2s
DNvec2 DN_vec2_subtract(DNvec2 v1, DNvec2 v2);
//Returns the result of a component-wise multiplication of 2 vector2s
DNvec2 DN_vec2_mult(DNvec2 v1, DNvec2 v2);
//Returns the result of a component-wise division of 2 vector2s
DNvec2 DN_vec2_divide(DNvec2 v1, DNvec2 v2);
//Returns the the result of scaling a vector2 by a scalar value
DNvec2 DN_vec2_scale(DNvec2 v, GLfloat s);
//Returns the result of taking each component to a power
DNvec2 DN_vec2_pow(DNvec2 v, GLfloat s);

//Returns a normalized vector2
DNvec2 DN_vec2_normalize(DNvec2 v);
//Clamps each value of the vector2 between min and max and returns the result
DNvec2 DN_vec2_clamp(DNvec2 v, GLfloat min, GLfloat max);
//Returns the result of a linear interpolation between 2 vector2s
DNvec2 DN_vec2_lerp(DNvec2 from, DNvec2 to, GLfloat a);

//Returns the length/magnitude of a vector2
GLfloat DN_vec2_length(DNvec2 v);
//Returns the dot product of 2 vector2s
GLfloat DN_vec2_dot(DNvec2 v1, DNvec2 v2);
//Returns the distance between 2 vector2s
GLfloat DN_vec2_distance(DNvec2 v1, DNvec2 v2);

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC3:

//Prints a vector3 to the console
void DN_vec3_print(DNvec3 v);

//Returns the sum of 2 vector3s
DNvec3 DN_vec3_add(DNvec3 v1, DNvec3 v2);
//Returns the difference of 2 vector3s
DNvec3 DN_vec3_subtract(DNvec3 v1, DNvec3 v2);
//Returns the result of a component-wise multiplication of 2 vector3s
DNvec3 DN_vec3_mult(DNvec3 v1, DNvec3 v2);
//Returns the result of a component-wise division of 2 vector3s
DNvec3 DN_vec3_divide(DNvec3 v1, DNvec3 v2);
//Returns the the result of scaling a vector3 by a scalar value
DNvec3 DN_vec3_scale(DNvec3 v, GLfloat s);
//Returns the result of taking each component to a power
DNvec3 DN_vec3_pow(DNvec3 v, GLfloat s);

//Returns a normalized vector3
DNvec3 DN_vec3_normalize(DNvec3 v);
//Clamps each value of the vector3 between min and max and returns the result
DNvec3 DN_vec3_clamp(DNvec3 v, GLfloat min, GLfloat max);
//Returns the result of a linear interpolation between 2 vector3s
DNvec3 DN_vec3_lerp(DNvec3 from, DNvec3 to, GLfloat a);
//Returns the cross product of 2 vector3s
DNvec3 DN_vec3_cross(DNvec3 v1, DNvec3 v2);

//Returns the length/magnitude of a vector3
GLfloat DN_vec3_length(DNvec3 v);
//Returns the dot product of 2 vector3s
GLfloat DN_vec3_dot(DNvec3 v1, DNvec3 v2);
//Returns the distance between 2 vector3s
GLfloat DN_vec3_distance(DNvec3 v1, DNvec3 v2);

//--------------------------------------------------------------------------------------------------------------------------------//
//VEC4:

//Prints a vector4 to the console
void DN_vec4_print(DNvec4 v);

//Returns the sum of 2 vector4s
DNvec4 DN_vec4_add(DNvec4 v1, DNvec4 v2);
//Returns the difference of 2 vector4s
DNvec4 DN_vec4_subtract(DNvec4 v1, DNvec4 v2);
//Returns the result of a component-wise multiplication of 2 vector4s
DNvec4 DN_vec4_mult(DNvec4 v1, DNvec4 v2);
//Returns the result of a component-wise division of 2 vector4s
DNvec4 DN_vec4_divide(DNvec4 v1, DNvec4 v2);
//Returns the the result of scaling a vector4 by a scalar value
DNvec4 DN_vec4_scale(DNvec4 v, GLfloat s);
//Returns the result of taking each component to a power
DNvec4 DN_vec4_pow(DNvec4 v, GLfloat s);

//Returns a normalized vector4
DNvec4 DN_vec4_normalize(DNvec4 v);
//Clamps each value of the vector4 between min and max and returns the result
DNvec4 DN_vec4_clamp(DNvec4 v, GLfloat min, GLfloat max);
//Returns the result of a linear interpolation between 2 vector4s
DNvec4 DN_vec4_lerp(DNvec4 from, DNvec4 to, GLfloat a);

//Returns the length/magnitude of a vector4
GLfloat DN_vec4_length(DNvec4 v);
//Returns the dot product of 2 vector4s
GLfloat DN_vec4_dot(DNvec4 v1, DNvec4 v2);
//Returns the distance between 2 vector4s
GLfloat DN_vec4_distance(DNvec4 v1, DNvec4 v2);

#endif