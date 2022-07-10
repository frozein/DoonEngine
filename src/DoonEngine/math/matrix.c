#include "matrix.h"
#include "quaternion.h"
#include "vector.h"
#include <stdio.h>
#include <math.h>

//Prints a matrix of NxN size
static void mat_print(unsigned int n, GLfloat* m);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAT3:

void DN_mat3_print(DNmat3 m)
{
	mat_print(3, &m.m[0][0]);
}

DNmat3 DN_mat3_mult(DNmat3 m1, DNmat3 m2)
{
	DNmat3 res;

	GLfloat a00 = m1.m[0][0], a01 = m1.m[0][1], a02 = m1.m[0][2],
    	    a10 = m1.m[1][0], a11 = m1.m[1][1], a12 = m1.m[1][2],
            a20 = m1.m[2][0], a21 = m1.m[2][1], a22 = m1.m[2][2],

            b00 = m2.m[0][0], b01 = m2.m[0][1], b02 = m2.m[0][2],
            b10 = m2.m[1][0], b11 = m2.m[1][1], b12 = m2.m[1][2],
            b20 = m2.m[2][0], b21 = m2.m[2][1], b22 = m2.m[2][2];

	res.m[0][0] = a00 * b00 + a10 * b01 + a20 * b02;
	res.m[0][1] = a01 * b00 + a11 * b01 + a21 * b02;
	res.m[0][2] = a02 * b00 + a12 * b01 + a22 * b02;
	res.m[1][0] = a00 * b10 + a10 * b11 + a20 * b12;
	res.m[1][1] = a01 * b10 + a11 * b11 + a21 * b12;
	res.m[1][2] = a02 * b10 + a12 * b11 + a22 * b12;
	res.m[2][0] = a00 * b20 + a10 * b21 + a20 * b22;
	res.m[2][1] = a01 * b20 + a11 * b21 + a21 * b22;
	res.m[2][2] = a02 * b20 + a12 * b21 + a22 * b22;

	return res;
}

DNvec3 DN_mat3_mult_vec3(DNmat3 m, DNvec3 v)
{
	DNvec3 res;

	res.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z;
	res.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z;
	res.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z;

	return res;
}

DNmat3 DN_mat3_scale_total(DNmat3 m, GLfloat s)
{
	DNmat3 res = m;

	res.m[0][0] *= s; res.m[0][1] *= s; res.m[0][2] *= s;
	res.m[1][0] *= s; res.m[1][1] *= s; res.m[1][2] *= s;
	res.m[2][0] *= s; res.m[2][1] *= s; res.m[2][2] *= s;

	return res;
}

DNmat3 DN_mat3_transpose(DNmat3 m)
{
	DNmat3 res;

	res.m[0][0] = m.m[0][0];
	res.m[0][1] = m.m[1][0];
	res.m[0][2] = m.m[2][0];
	res.m[1][0] = m.m[0][1];
	res.m[1][1] = m.m[1][1];
	res.m[1][2] = m.m[2][1];
	res.m[2][0] = m.m[0][2];
	res.m[2][1] = m.m[1][2];
	res.m[2][2] = m.m[2][2];

	return res;
}

GLfloat DN_mat3_det(DNmat3 mat)
{
	GLfloat a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2],
          d = mat.m[1][0], e = mat.m[1][1], f = mat.m[1][2],
          g = mat.m[2][0], h = mat.m[2][1], i = mat.m[2][2];

	return a * (e * i - h * f) - d * (b * i - c * h) + g * (b * f - c * e);
}

DNmat3 DN_mat3_inv(DNmat3 mat)
{
	DNmat3 res;

	GLfloat det;
  	GLfloat a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2],
            d = mat.m[1][0], e = mat.m[1][1], f = mat.m[1][2],
            g = mat.m[2][0], h = mat.m[2][1], i = mat.m[2][2];

	res.m[0][0] =   e * i - f * h;
	res.m[0][1] = -(b * i - h * c);
	res.m[0][2] =   b * f - e * c;
	res.m[1][0] = -(d * i - g * f);
	res.m[1][1] =   a * i - c * g;
	res.m[1][2] = -(a * f - d * c);
	res.m[2][0] =   d * h - g * e;
	res.m[2][1] = -(a * h - g * b);
	res.m[2][2] =   a * e - b * d;

	det = 1.0f / (a * res.m[0][0] + b * res.m[1][0] + c * res.m[2][0]);

	return DN_mat3_scale_total(res, det);
}

DNmat3 DN_mat3_translate(DNmat3 m, DNvec2 dir)
{
	DNmat3 translated = DN_MAT3_IDENTITY;
	translated.m[2][0] = dir.x;
	translated.m[2][1] = dir.y;

	return DN_mat3_mult(m, translated);
}

DNmat3 DN_mat3_scale(DNmat3 m, DNvec2 s)
{
	DNmat3 scaled = DN_MAT3_IDENTITY;
	scaled.m[0][0] = s.x;
	scaled.m[1][1] = s.y;

	return DN_mat3_mult(m, scaled);
}

DNmat3 DN_mat3_rotate(DNmat3 m, GLfloat angle)
{
	DNmat3 rotated;

	GLfloat sine =   sinf(angle * DEG_TO_RAD);
	GLfloat cosine = cosf(angle * DEG_TO_RAD);
	rotated.m[0][0] = cosine;
	rotated.m[1][0] =  sine;
	rotated.m[0][1] = -sine;
	rotated.m[1][1] = cosine;

	return DN_mat3_mult(m, rotated);
}

//--------------------------------------------------------------------------------------------------------------------------------//
//MAT4:

void DN_mat4_print(DNmat4 m)
{
	mat_print(4, &m.m[0][0]);
}

DNmat4 DN_mat4_mult(DNmat4 m1, DNmat4 m2)
{
	DNmat4 res;

	GLfloat a00 = m1.m[0][0], a01 = m1.m[0][1], a02 = m1.m[0][2], a03 = m1.m[0][3], //LOL (expanded so that its faster)
            a10 = m1.m[1][0], a11 = m1.m[1][1], a12 = m1.m[1][2], a13 = m1.m[1][3],
            a20 = m1.m[2][0], a21 = m1.m[2][1], a22 = m1.m[2][2], a23 = m1.m[2][3],
            a30 = m1.m[3][0], a31 = m1.m[3][1], a32 = m1.m[3][2], a33 = m1.m[3][3],

            b00 = m2.m[0][0], b01 = m2.m[0][1], b02 = m2.m[0][2], b03 = m2.m[0][3],
            b10 = m2.m[1][0], b11 = m2.m[1][1], b12 = m2.m[1][2], b13 = m2.m[1][3],
            b20 = m2.m[2][0], b21 = m2.m[2][1], b22 = m2.m[2][2], b23 = m2.m[2][3],
            b30 = m2.m[3][0], b31 = m2.m[3][1], b32 = m2.m[3][2], b33 = m2.m[3][3];

	res.m[0][0] = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
	res.m[0][1] = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
	res.m[0][2] = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
	res.m[0][3] = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
	res.m[1][0] = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
	res.m[1][1] = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
	res.m[1][2] = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
	res.m[1][3] = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
	res.m[2][0] = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
	res.m[2][1] = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
	res.m[2][2] = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
	res.m[2][3] = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
	res.m[3][0] = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
	res.m[3][1] = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
	res.m[3][2] = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
	res.m[3][3] = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;

	return res;
}

DNvec4 DN_mat4_mult_vec4(DNmat4 m, DNvec4 v)
{
	DNvec4 res;

	res.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;
	res.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;
	res.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;
	res.w = m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

	return res;
}

DNmat4 DN_mat4_scale_total(DNmat4 m, GLfloat s)
{
	DNmat4 res = m;

	res.m[0][0] *= s; res.m[0][1] *= s; res.m[0][2] *= s; res.m[0][3] *= s;
	res.m[1][0] *= s; res.m[1][1] *= s; res.m[1][2] *= s; res.m[1][3] *= s;
	res.m[2][0] *= s; res.m[2][1] *= s; res.m[2][2] *= s; res.m[2][3] *= s;
	res.m[3][0] *= s; res.m[3][1] *= s; res.m[3][2] *= s; res.m[3][3] *= s;

	return res;
}

DNmat4 DN_mat4_transpose(DNmat4 m)
{
	DNmat4 res;

	res.m[0][0] = m.m[0][0]; res.m[1][0] = m.m[0][1];
	res.m[0][1] = m.m[1][0]; res.m[1][1] = m.m[1][1];
	res.m[0][2] = m.m[2][0]; res.m[1][2] = m.m[2][1];
	res.m[0][3] = m.m[3][0]; res.m[1][3] = m.m[3][1];
	res.m[2][0] = m.m[0][2]; res.m[3][0] = m.m[0][3];
	res.m[2][1] = m.m[1][2]; res.m[3][1] = m.m[1][3];
	res.m[2][2] = m.m[2][2]; res.m[3][2] = m.m[2][3];
	res.m[2][3] = m.m[3][2]; res.m[3][3] = m.m[3][3];

	return res;
}

GLfloat DN_mat4_det(DNmat4 mat)
{
	GLfloat t[6];
	GLfloat a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2], d = mat.m[0][3],
    	    e = mat.m[1][0], f = mat.m[1][1], g = mat.m[1][2], h = mat.m[1][3],
    	    i = mat.m[2][0], j = mat.m[2][1], k = mat.m[2][2], l = mat.m[2][3],
    	    m = mat.m[3][0], n = mat.m[3][1], o = mat.m[3][2], p = mat.m[3][3];

	t[0] = k * p - o * l;
	t[1] = j * p - n * l;
	t[2] = j * o - n * k;
	t[3] = i * p - m * l;
	t[4] = i * o - m * k;
	t[5] = i * n - m * j;

  	return a * (f * t[0] - g * t[1] + h * t[2])
         - b * (e * t[0] - g * t[3] + h * t[4])
         + c * (e * t[1] - f * t[3] + h * t[5])
         - d * (e * t[2] - f * t[4] + g * t[5]);
}

DNmat4 DN_mat4_inv(DNmat4 mat)
{
	DNmat4 res;

	GLfloat t[6];
	GLfloat det;
	GLfloat a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2], d = mat.m[0][3],
            e = mat.m[1][0], f = mat.m[1][1], g = mat.m[1][2], h = mat.m[1][3],
            i = mat.m[2][0], j = mat.m[2][1], k = mat.m[2][2], l = mat.m[2][3],
            m = mat.m[3][0], n = mat.m[3][1], o = mat.m[3][2], p = mat.m[3][3];

	t[0] = k * p - o * l; 
	t[1] = j * p - n * l; 
	t[2] = j * o - n * k;
	t[3] = i * p - m * l; 
	t[4] = i * o - m * k; 
	t[5] = i * n - m * j;

	res.m[0][0] =  f * t[0] - g * t[1] + h * t[2];
	res.m[1][0] =-(e * t[0] - g * t[3] + h * t[4]);
	res.m[2][0] =  e * t[1] - f * t[3] + h * t[5];
	res.m[3][0] =-(e * t[2] - f * t[4] + g * t[5]);

	res.m[0][1] =-(b * t[0] - c * t[1] + d * t[2]);
	res.m[1][1] =  a * t[0] - c * t[3] + d * t[4];
	res.m[2][1] =-(a * t[1] - b * t[3] + d * t[5]);
	res.m[3][1] =  a * t[2] - b * t[4] + c * t[5];

	t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
	t[3] = e * p - m * h; t[4] = e * o - m * g; t[5] = e * n - m * f;

	res.m[0][2] =  b * t[0] - c * t[1] + d * t[2];
	res.m[1][2] =-(a * t[0] - c * t[3] + d * t[4]);
	res.m[2][2] =  a * t[1] - b * t[3] + d * t[5];
	res.m[3][2] =-(a * t[2] - b * t[4] + c * t[5]);

	t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
	t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;

	res.m[0][3] =-(b * t[0] - c * t[1] + d * t[2]);
	res.m[1][3] =  a * t[0] - c * t[3] + d * t[4];
	res.m[2][3] =-(a * t[1] - b * t[3] + d * t[5]);
  	res.m[3][3] =  a * t[2] - b * t[4] + c * t[5];

  	det = 1.0f / (a * res.m[0][0] + b * res.m[1][0]
                + c * res.m[2][0] + d * res.m[3][0]);

  	return DN_mat4_scale_total(res, det);
}

DNmat3 DN_mat4_to_mat3(DNmat4 m)
{
	DNmat3 res;

	res.m[0][0] = m.m[0][0];
	res.m[1][0] = m.m[1][0];
	res.m[2][0] = m.m[2][0];
	res.m[0][1] = m.m[0][1];
	res.m[1][1] = m.m[1][1];
	res.m[2][1] = m.m[2][1];
	res.m[0][2] = m.m[0][2];
	res.m[1][2] = m.m[1][2];
	res.m[2][2] = m.m[2][2];

	return res;
}

DNmat4 DN_mat4_translate(DNmat4 m, DNvec3 dir)
{	
	DNmat4 translated = DN_MAT4_IDENTITY;
	translated.m[3][0] = dir.x;
	translated.m[3][1] = dir.y;
	translated.m[3][2] = dir.z;

	return DN_mat4_mult(m, translated);
}

DNmat4 DN_mat4_scale(DNmat4 m, DNvec3 s)
{
	DNmat4 scaled = DN_MAT4_IDENTITY;
	scaled.m[0][0] = s.x;
	scaled.m[1][1] = s.y;
	scaled.m[2][2] = s.z;

	return DN_mat4_mult(m, scaled);
}

DNmat4 DN_mat4_rotate(DNmat4 m, DNvec3 axis, GLfloat angle)
{
	return DN_mat4_mult(m, DN_quat_get_mat4(DN_quat_from_axis_angle(axis, angle)));
}

DNmat4 DN_mat4_rotate_euler(DNmat4 m, DNvec3 angles)
{
	return DN_mat4_mult(m, DN_quat_get_mat4(DN_quat_from_euler(angles)));
}

DNmat4 DN_mat4_perspective_proj(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	DNmat4 res = DN_MAT4_ZERO;

	res.m[0][0] = n / r;
	res.m[1][1] = n / t;
	res.m[2][2] = -(f + n) / (f - n);
	res.m[3][2] = -2 * f * n / (f - n);
	res.m[2][3] = -1;

	return res;
}

DNmat4 DN_mat4_perspective_proj_from_fov(GLfloat fov, GLfloat aspect, GLfloat n, GLfloat f)
{
	GLfloat scale = tanf(fov * 0.5 * DEG_TO_RAD) * n; 
    GLfloat r = aspect * scale;
    GLfloat t = scale;

	return DN_mat4_perspective_proj(-r, r, -t, t, n, f);
}

DNmat4 DN_mat4_lookat(DNvec3 pos, DNvec3 target, DNvec3 up)
{
	DNvec3 d = DN_vec3_normalize(DN_vec3_subtract(pos, target)); //direction
	DNvec3 r = DN_vec3_normalize(DN_vec3_cross(up, d)); //right axis
	DNvec3 u = DN_vec3_cross(d, r); //up axis

	DNmat4 RUD = DN_MAT4_IDENTITY;
	RUD.m[0][0] = r.x;
	RUD.m[1][0] = r.y;
	RUD.m[2][0] = r.z;
	RUD.m[0][1] = u.x;
	RUD.m[1][1] = u.y;
	RUD.m[2][1] = u.z;
	RUD.m[0][2] = d.x;
	RUD.m[1][2] = d.y;
	RUD.m[2][2] = d.z;

	DNmat4 translate = DN_MAT4_IDENTITY;
	translate.m[3][0] = -pos.x;
	translate.m[3][1] = -pos.y;
	translate.m[3][2] = -pos.z;

	return DN_mat4_mult(RUD, translate);
}

//--------------------------------------------------------------------------------------------------------------------------------//

void mat_print(unsigned int n, GLfloat* m)
{
	for(int i = 0; i < n; i++)
	{
		for(int j = 0; j < n; j++)
		{
			GLfloat val = m[j * n + i];

			if(val == 0.0f) //to avoid -0 causing it to print out misalligned
				val = 0.0f;

			if(val >= 0.0f)
				printf(" ");

			if(j == n - 1)
				printf("%f", val); //TODO: ensure that can be printed even if GLfloat isnt a float
			else
				printf("%f, ", val);
		}

		printf("\n");
	}
}