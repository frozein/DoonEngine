#ifndef DN_QUATERNION_H
#define DN_QUATERNION_H

#include "common.h"
#include "matrix.h"

//--------------------------------------------------------------------------------------------------------------------------------//

//Prints a quaternion to the console
void DN_quat_print(DNquat q);

//Returns the sum of 2 quaternions
DNquat DN_quat_add(DNquat q1, DNquat q2);
//Returns the difference of 2 quaternions
DNquat DN_quat_subtract(DNquat q1, DNquat q2);
//Returns the result of scaling a quaternion by a scalar value
DNquat DN_quat_scale(DNquat q1, GLfloat s);
//Returns the product of 2 quaternions
DNquat DN_quat_mult(DNquat q1, DNquat q2);

//Returns the conjugate of a quaternion
DNquat DN_quat_conjugate(DNquat q);

//Returns the quaternion representing the rotation about an axis
DNquat DN_quat_from_axis_angle(DNvec3 axis, GLfloat angle);
//Returns the quaternion representing a euler rotation
DNquat DN_quat_from_euler(DNvec3 angles);

//Returns the dot product of 2 quaternions
GLfloat DN_quat_dot(DNquat q1, DNquat q2);
//Returns the result of a spherical linear interpolation between 2 quaternions
DNquat DN_quat_slerp(DNquat from, DNquat to, GLfloat a);

//Casts a quaternion to a vector4
DNvec4 DN_quat_to_vec4(DNquat q);
//Casts a vector4 to a quaternion
DNquat DN_vec4_to_quat(DNvec4 v);
//Returns the 4x4 transformation matrix representing a quaternion rotation
DNmat4 DN_quat_get_mat4(DNquat q);

#endif