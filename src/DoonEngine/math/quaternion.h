#ifndef DN_QUATERNION_H
#define DN_QUATERNION_H

#include "common.h"
#include "matrix.h"

//--------------------------------------------------------------------------------------------------------------------------------//

//Prints a quaternion to the console
void quat_print(quat q);

//Returns the sum of 2 quaternions
quat quat_add(quat q1, quat q2);
//Returns the difference of 2 quaternions
quat quat_subtract(quat q1, quat q2);
//Returns the result of scaling a quaternion by a scalar value
quat quat_scale(quat q1, GLfloat s);
//Returns the product of 2 quaternions
quat quat_mult(quat q1, quat q2);

//Returns the conjugate of a quaternion
quat quat_conjugate(quat q);

//Returns the quaternion representing the rotation about an axis
quat quat_from_axis_angle(vec3 axis, GLfloat angle);
//Returns the quaternion representing a euler rotation
quat quat_from_euler(vec3 angles);

//Returns the dot product of 2 quaternions
GLfloat quat_dot(quat q1, quat q2);
//Returns the result of a spherical linear interpolation between 2 quaternions
quat quat_slerp(quat from, quat to, GLfloat a);

//Casts a quaternion to a vector4
vec4 quat_to_vec4(quat q);
//Casts a vector4 to a quaternion
quat vec4_to_quat(vec4 v);
//Returns the 4x4 transformation matrix representing a quaternion rotation
mat4 quat_get_mat4(quat q);

#endif