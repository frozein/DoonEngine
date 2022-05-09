#include "quaternion.h"
#include "vector.h"
#include <stdio.h>
#include <math.h>

//--------------------------------------------------------------------------------------------------------------------------------//

void DN_quat_print(DNquat q)
{
	DN_vec4_print(DN_quat_to_vec4(q));
}

DNquat DN_quat_add(DNquat q1, DNquat q2)
{
	return (DNquat){q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w};
}

DNquat DN_quat_subtract(DNquat q1, DNquat q2)
{
	return (DNquat){q1.x - q2.x, q1.y - q2.y, q1.z - q2.z, q1.w - q2.w};
}

DNquat DN_quat_scale(DNquat q1, GLfloat s)
{
	return (DNquat){q1.x * s, q1.y * s, q1.z * s, q1.w * s};
}

DNquat DN_quat_mult(DNquat q1, DNquat q2)
{
	DNquat res;

    res.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    res.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    res.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    res.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

	return res;
}

DNquat DN_quat_from_axis_angle(DNvec3 axis, GLfloat angle)
{
	DNquat res;

	angle *= 0.5f * DEG_TO_RAD;
	DNvec3 normalized = DN_vec3_normalize(axis);
	GLfloat sine = sinf(angle);
	res.w = cosf(angle);
	res.x = normalized.x * sine;
	res.y = normalized.y * sine;
	res.z = normalized.z * sine;

	return res;
}

DNquat DN_quat_from_euler(DNvec3 angles)
{
	DNquat res;

	angles.x *= 0.5f * DEG_TO_RAD;
	angles.y *= 0.5f * DEG_TO_RAD;
	angles.z *= 0.5f * DEG_TO_RAD;

	GLfloat sinx = sinf(angles.x); 
	GLfloat siny = sinf(angles.y);
	GLfloat sinz = sinf(angles.z); 
	GLfloat cosx = cosf(angles.x);
	GLfloat cosy = cosf(angles.y); 
	GLfloat cosz = cosf(angles.z); 

	res.x = sinx * cosy * cosz - cosx * siny * sinz;
	res.y = cosx * siny * cosz + sinx * cosy * sinz;
	res.z = cosx * cosy * sinz - sinx * siny * cosz;
	res.w = cosx * cosy * cosz + sinx * siny * sinz;
	return res;
}

DNquat DN_quat_conjugate(DNquat q)
{
	DNquat res = q;
	res.x = -res.x;
	res.y = -res.y;
	res.z = -res.z;

	return res;
}

GLfloat DN_quat_dot(DNquat q1, DNquat q2)
{
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

DNquat DN_quat_slerp(DNquat from, DNquat to, GLfloat a)
{
	DNquat q1, q2;
	GLfloat cosTheta, sinTheta, angle;

	cosTheta = DN_quat_dot(from, to);
	q1 = from;

	if (fabsf(cosTheta) >= 1.0f) 
		return from;

	if (cosTheta < 0.0f) 
	{
		q1 = (DNquat){-q1.x, -q1.y, -q1.z, -q1.w};
    	cosTheta = -cosTheta;
	}

	sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

	if (fabsf(sinTheta) < 0.001f) 
	{
    	return DN_vec4_to_quat(DN_vec4_lerp(DN_quat_to_vec4(from), DN_quat_to_vec4(to), a));
	}

	angle = acosf(cosTheta);
	q1 = DN_quat_scale(q1, sinf((1.0f - a) * angle));
	q2 = DN_quat_scale(to, sinf(a * angle));

	q1 = DN_quat_add(q1, q2);
	return DN_quat_scale(q1, 1.0f / sinTheta);
}

DNvec4 DN_quat_to_vec4(DNquat q)
{
	return (DNvec4){q.x, q.y, q.z, q.w};
}

DNquat DN_vec4_to_quat(DNvec4 v)
{
	return (DNquat){v.x, v.y, v.z, v.w};
}

DNmat4 DN_quat_get_mat4(DNquat q)
{
	GLfloat x2  = q.x + q.x;
    GLfloat y2  = q.y + q.y;
    GLfloat z2  = q.z + q.z;
    GLfloat xx2 = q.x * x2;
    GLfloat xy2 = q.x * y2;
    GLfloat xz2 = q.x * z2;
    GLfloat yy2 = q.y * y2;
    GLfloat yz2 = q.y * z2;
    GLfloat zz2 = q.z * z2;
    GLfloat sx2 = q.w * x2;
    GLfloat sy2 = q.w * y2;
    GLfloat sz2 = q.w * z2;

	return (DNmat4) {{{1 - (yy2 + zz2),  xy2 + sz2,        xz2 - sy2,        0},
                   {xy2 - sz2,        1 - (xx2 + zz2),  yz2 + sx2,        0},
                   {xz2 + sy2,        yz2 - sx2,        1 - (xx2 + yy2),  0},
                   {0,                0,                0,                1}}};
}