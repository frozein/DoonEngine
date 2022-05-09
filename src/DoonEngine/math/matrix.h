#ifndef DN_MATRIX_H
#define DN_MATRIX_H

#include "common.h"

//IMPORTANT: ALL ANGLES IN DEGREES

//--------------------------------------------------------------------------------------------------------------------------------//
//MAT3:

//Prints a 3x3 matrix to the console
void DN_mat3_print(DNmat3 m);

//Returns the product of 2 3x3 matrices
DNmat3 DN_mat3_mult(DNmat3 m1, DNmat3 m2);
//Returns the product of a 3x3 matrix and a vector4
DNvec3 DN_mat3_mult_vec3(DNmat3 m, DNvec3 v);

//Returns a matrix with every value scaled by a scalar value
DNmat3 DN_mat3_scale_total(DNmat3 m, GLfloat s);
//Returns the transpose of a matrix
DNmat3 DN_mat3_transpose(DNmat3 m);
//Returns the determinant of a 3x3 matrix
GLfloat DN_mat3_det(DNmat3 m);
//Returns the inverse of a 3x3 matrix
DNmat3 DN_mat3_inv(DNmat3 m);

//Returns the translated version of a 3x3 transformation matrix (only affects x and y)
DNmat3 DN_mat3_translate(DNmat3 m, DNvec2 dir);
//Returns the scaled version of a 3x3 transformation matrix (only affects x and y)
DNmat3 DN_mat3_scale(DNmat3 m, DNvec2 s);
//Returns the rotated version of a 3x3 transformation matrix (only affects x and y)
DNmat3 DN_mat3_rotate(DNmat3 m, GLfloat angle);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAT4:

//Prints a 4x4 matrix to the console
void DN_mat4_print(DNmat4 m);

//Returns the product of 2 4x4 matrices
DNmat4 DN_mat4_mult(DNmat4 m1, DNmat4 m2);
//Returns the product of a 4x4 matrix and a vector4
DNvec4 DN_mat4_mult_vec4(DNmat4 m, DNvec4 v);

//Returns a matrix with every value scaled by a scalar value
DNmat4 DN_mat4_scale_total(DNmat4 m, GLfloat s);
//Returns the transpose of a matrix
DNmat4 DN_mat4_transpose(DNmat4 m);
//Returns the determinant of a 4x4 matrix
GLfloat DN_mat4_det(DNmat4 m);
//Returns the inverse of a 4x4 matrix
DNmat4 DN_mat4_inv(DNmat4 m);

//Returns the upper left corner of a 4x4 matrix as a 3x3 matrix
DNmat3 DN_mat4_to_mat3(DNmat4 m);

//Returns the translated version of a 4x4 transformation matrix (only affects x, y, and z)
DNmat4 DN_mat4_translate(DNmat4 m, DNvec3 dir);
//Returns the scaled version of a 4x4 transformation matrix (only affects x, y, and z)
DNmat4 DN_mat4_scale(DNmat4 m, DNvec3 s);
//Returns the rotated version of a 4x4 transformation matrix (only affects x, y, and z)
DNmat4 DN_mat4_rotate(DNmat4 m, DNvec3 axis, GLfloat angle);
//Returns the rotated version of a 4x4 transformation matrix (only affects x, y, and z)
DNmat4 DN_mat4_rotate_euler(DNmat4 m, DNvec3 angles);

//Returns a perspective projection matrix from a left, right, bottom, and top bound as well as a near and far clip plane
DNmat4 DN_mat4_perspective_proj(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);
//Returns a perspective projection matrix from an fov, aspect ratio, and near and far clip planes
DNmat4 DN_mat4_perspective_proj_from_fov(GLfloat fov, GLfloat aspect, GLfloat near, GLfloat far);
//Generates a view matrix from a camera position, camera target, and up vector
DNmat4 DN_mat4_lookat(DNvec3 pos, DNvec3 target, DNvec3 up);

#endif