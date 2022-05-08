#include "quaternion.h"
#include "vector.h"
#include <stdio.h>
#include <math.h>

//--------------------------------------------------------------------------------------------------------------------------------//

void quat_print(DNquat q)
{
	vec4_print(quat_to_vec4(q));
}

DNquat quat_add(DNquat q1, DNquat q2)
{
	return (DNquat){q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w};
}

DNquat quat_subtract(DNquat q1, DNquat q2)
{
	return (DNquat){q1.x - q2.x, q1.y - q2.y, q1.z - q2.z, q1.w - q2.w};
}

DNquat quat_scale(DNquat q1, GLfloat s)
{
	return (DNquat){q1.x * s, q1.y * s, q1.z * s, q1.w * s};
}

DNquat quat_mult(DNquat q1, DNquat q2)
{
	DNquat res;

    res.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    res.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    res.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    res.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

	return res;
}

DNquat quat_from_axis_angle(DNvec3 axis, GLfloat angle)
{
	DNquat res;

	angle *= 0.5f * DEG_TO_RAD;
	DNvec3 normalized = vec3_normalize(axis);
	GLfloat sine = sinf(angle);
	res.w = cosf(angle);
	res.x = normalized.x * sine;
	res.y = normalized.y * sine;
	res.z = normalized.z * sine;

	return res;
}

DNquat quat_from_euler(DNvec3 angles)
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

DNquat quat_conjugate(DNquat q)
{
	DNquat res = q;
	res.x = -res.x;
	res.y = -res.y;
	res.z = -res.z;

	return res;
}

GLfloat quat_dot(DNquat q1, DNquat q2)
{
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

DNquat quat_slerp(DNquat from, DNquat to, GLfloat a)
{
	DNquat q1, q2;
	GLfloat cosTheta, sinTheta, angle;

	cosTheta = quat_dot(from, to);
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
    	return vec4_to_quat(vec4_lerp(quat_to_vec4(from), quat_to_vec4(to), a));
	}

	angle = acosf(cosTheta);
	q1 = quat_scale(q1, sinf((1.0f - a) * angle));
	q2 = quat_scale(to, sinf(a * angle));

	q1 = quat_add(q1, q2);
	return quat_scale(q1, 1.0f / sinTheta);
}

DNvec4 quat_to_vec4(DNquat q)
{
	return (DNvec4){q.x, q.y, q.z, q.w};
}

DNquat vec4_to_quat(DNvec4 v)
{
	return (DNquat){v.x, v.y, v.z, v.w};
}

DNmat4 quat_get_mat4(DNquat q)
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