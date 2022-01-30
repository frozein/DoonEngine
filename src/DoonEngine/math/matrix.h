#ifndef DN_MATRIX_H
#define DN_MATRIX_H

#include "common.h"

//IMPORTANT: ALL ANGLES IN DEGREES

//--------------------------------------------------------------------------------------------------------------------------------//
//MAT3:

//Prints a 3x3 matrix to the console
void mat3_print(mat3 m);

//Returns the product of 2 3x3 matrices
mat3 mat3_mult(mat3 m1, mat3 m2);
//Returns the product of a 3x3 matrix and a vector4
vec3 mat3_mult_vec3(mat3 m, vec3 v);

//Returns a matrix with every value scaled by a scalar value
mat3 mat3_scale_total(mat3 m, GLfloat s);
//Returns the transpose of a matrix
mat3 mat3_transpose(mat3 m);
//Returns the determinant of a 3x3 matrix
float mat3_det(mat3 m);
//Returns the inverse of a 3x3 matrix
mat3 mat3_inv(mat3 m);

//Returns the translated version of a 3x3 transformation matrix (only affects x and y)
mat3 mat3_translate(mat3 m, vec2 dir);
//Returns the scaled version of a 3x3 transformation matrix (only affects x and y)
mat3 mat3_scale(mat3 m, vec2 s);
//Returns the rotated version of a 3x3 transformation matrix (only affects x and y)
mat3 mat3_rotate(mat3 m, GLfloat angle);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAT4:

//Prints a 4x4 matrix to the console
void mat4_print(mat4 m);

//Returns the product of 2 4x4 matrices
mat4 mat4_mult(mat4 m1, mat4 m2);
//Returns the product of a 4x4 matrix and a vector4
vec4 mat4_mult_vec4(mat4 m, vec4 v);

//Returns a matrix with every value scaled by a scalar value
mat4 mat4_scale_total(mat4 m, GLfloat s);
//Returns the transpose of a matrix
mat4 mat4_transpose(mat4 m);
//Returns the determinant of a 4x4 matrix
float mat4_det(mat4 m);
//Returns the inverse of a 4x4 matrix
mat4 mat4_inv(mat4 m);

//Returns the upper left corner of a 4x4 matrix as a 3x3 matrix
mat3 mat4_to_mat3(mat4 m);

//Returns the translated version of a 4x4 transformation matrix (only affects x, y, and z)
mat4 mat4_translate(mat4 m, vec3 dir);
//Returns the scaled version of a 4x4 transformation matrix (only affects x, y, and z)
mat4 mat4_scale(mat4 m, vec3 s);
//Returns the rotated version of a 4x4 transformation matrix (only affects x, y, and z)
mat4 mat4_rotate(mat4 m, vec3 axis, GLfloat angle);
//Returns the rotated version of a 4x4 transformation matrix (only affects x, y, and z)
mat4 mat4_rotate_euler(mat4 m, vec3 angles);

//Returns a perspective projection matrix from a left, right, bottom, and top bound as well as a near and far clip plane
mat4 mat4_perspective_proj(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);
//Returns a perspective projection matrix from an fov, aspect ratio, and near and far clip planes
mat4 mat4_perspective_proj_from_fov(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far);
//Generates a view matrix from a camera position, camera target, and up vector
mat4 mat4_lookat(vec3 pos, vec3 target, vec3 up);

#endif