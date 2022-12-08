/* ------------------------------------------------------------------------
 *
 * quickmath.h
 * author: Daniel Elwell (2022)
 * license: MIT
 * description: a single-header library for common vector, matrix, and quaternion math
 * functions designed for games and graphics programming.
 * 
 * ------------------------------------------------------------------------
 * 
 * to change or disable the function prefix (the default is "QM_"), change the macro on
 * line 98 to contain the desired prefix, or "#define QM_PREFIX" for no prefix
 * 
 * if you wish not to use SSE3 intrinsics (if they are not supported for example),
 * change the macro on line 88 to "#define QM_USE_SSE 0"
 * 
 * to disable the need to link with the C runtime library, change the macros beginning
 * on line 104 and the #include on line 102 to the appropirate functions/files
 * 
 * ------------------------------------------------------------------------
 * 
 * the following functions are defined:
 * (QMvecn means a vector of dimension, 2, 3, or 4, named QMvec2, QMvec3, and QMvec4)
 * (QMmatn means a matrix of dimensions 3x3 or 4x4, named QMmat3 and QMmat4)
 * 
 * QMvecn       QM_vecn_add                   (QMvecn v1, QMvecn v2);
 * QMvecn       QM_vecn_sub                   (QMvecn v1, QMvecn v2);
 * QMvecn       QM_vecn_mult                  (QMvecn v1, QMvecn v2);
 * QMvecn       QM_vecn_div                   (QMvecn v1, QMvecn v2);
 * QMvecn       QM_vecn_scale                 (QMvecn v , float  s );
 * QMvecn       QM_vecn_dot                   (QMvecn v1, QMvecn v2);
 * QMvec3       QM_vec3_cross                 (QMvec3 v1, QMvec3 v2);
 * float        QM_vecn_length                (QMvecn v);
 * QMvecn       QM_vecn_normalize             (QMvecn v);
 * float        QM_vecn_distance              (QMvecn v1, QMvecn v2);
 * int          QM_vecn_equals                (QMvecn v1, QMvecn v2);
 * QMvecn       QM_vecn_min                   (QMvecn v1, QMvecn v2);
 * QMvecn       QM_vecn_max                   (QMvecn v1, QMvecn v2);
 * 
 * QMmatn       QM_matn_identity              ();
 * QMmatn       QM_matn_add                   (QMmatn m1, QMmatn m2);
 * QMmatn       QM_matn_sub                   (QMmatn m1, QMmatn m2);
 * QMmatn       QM_matn_mult                  (QMmatn m1, QMmatn m2);
 * QMvecn       QM_matn_mult_vecn             (QMmatn m , QMvecn v );
 * QMmatn       QM_matn_transpose             (QMmatn m);
 * QMmatn       QM_matn_inv                   (QMmatn m);
 * 
 * QMmat3       QM_mat3_translate             (QMvec2 t);
 * QMmat4       QM_mat4_translate             (QMvec3 t);
 * QMmat3       QM_mat3_scale                 (QMvec2 s);
 * QMmat4       QM_mat4_scale                 (QMvec3 s);
 * QMmat3       QM_mat3_rotate                (float angle);
 * QMmat4       QM_mat4_rotate                (QMvec3 axis, float angle);
 * QMmat4       QM_mat4_rotate_euler          (QMvec3 angles);
 * QMmat3       QM_mat4_top_left              (QMmat4 m);
 *
 * QMmat4       QM_mat4_prespective           (float fov, float aspect, float near, float far);
 * QMmat4       QM_mat4_orthographic          (float left, float right, float bot, float top, float near, float far);
 * QMmat4       QM_mat4_look                  (QMvec3 pos, QMvec3 dir   , QMvec3 up);
 * QMmat4       QM_mat4_lookat                (QMvec3 pos, QMvec3 target, QMvec3 up);
 * 
 * QMquaternion QM_quaternion_identity        ();
 * QMquaternion QM_quaternion_add             (QMquaternion q1, QMquaternion q2);
 * QMquaternion QM_quaternion_sub             (QMquaternion q1, QMquaternion q2);
 * QMquaternion QM_quaternion_mult            (QMquaternion q1, QMquaternion q2);
 * QMquaternion QM_quaternion_scale           (QMquaternion q, float s);
 * QMquaternion QM_quaternion_dot             (QMquaternion q1, QMquaternion q2);
 * float        QM_quaternion_length          (QMquaternion q);
 * QMquaternion QM_quaternion_normalize       (QMquaternion q);
 * QMquaternion QM_quaternion_conjugate       (QMquaternion q);
 * QMquaternion QM_quaternion_inv             (QMquaternion q);
 * QMquaternion QM_quaternion_slerp           (QMquaternion q1, QMquaternion q2, float a);
 * QMquaternion QM_quaternion_from_axis_angle (QMvec3 axis, float angle);
 * QMquaternion QM_quaternion_from_euler      (QMvec3 angles);
 * QMmat4       QM_quaternion_to_mat4         (QMquaternion q);
 */

#ifndef QM_MATH_H
#define QM_MATH_H

#ifdef __cplusplus
extern "C"
{
#endif

//if you wish NOT to use SSE3 SIMD intrinsics, simply change the
//#define to 0
#define QM_USE_SSE 1
#if QM_USE_SSE
	#include <xmmintrin.h>
	#include <pmmintrin.h>
#endif

#define QM_INLINE static inline

//if you wish to set your own function prefix or remove it entirely,
//simply change this macro:
#define QM_PREFIX(name) DN_##name

//if you wish to not use any of the CRT functions, you must #define your
//own versions of the below functions and #include the appropriate header
#include <math.h>

#define QM_SQRTF sqrtf
#define QM_SINF  sinf
#define QM_COSF  cosf
#define QM_TANF  tanf
#define QM_ACOSF acosf

//----------------------------------------------------------------------//
//STRUCT DEFINITIONS:

typedef int QMbool;

//a 2-dimensional vector of floats
typedef union
{
	float v[2];
	struct{ float x, y; };
	struct{ float w, h; };
} QMvec2;

//a 3-dimensional vector of floats
typedef union
{
	float v[3];
	struct{ float x, y, z; };
	struct{ float w, h, d; };
	struct{ float r, g, b; };
} QMvec3;

//a 4-dimensional vector of floats
typedef union
{
	float v[4];
	struct{ float x, y, z, w; };
	struct{ float r, g, b, a; };

	#if QM_USE_SSE

	__m128 packed;

	#endif
} QMvec4;

//-----------------------------//
//matrices are column-major

typedef union
{
	float m[3][3];
} QMmat3;

typedef union
{
	float m[4][4];

	#if QM_USE_SSE

	__m128 packed[4]; //array of columns

	#endif
} QMmat4;

//-----------------------------//

typedef union
{
	float q[4];
	struct{ float x, y, z, w; };

	#if QM_USE_SSE

	__m128 packed;

	#endif
} QMquaternion;

//typedefs:
typedef QMvec2 DNvec2;
typedef QMvec3 DNvec3;
typedef QMvec4 DNvec4;
typedef QMmat3 DNmat3;
typedef QMmat4 DNmat4;
typedef QMquaternion DNquaternion;

//----------------------------------------------------------------------//
//HELPER FUNCS:

#define QM_MIN(x, y) ((x) < (y) ? (x) : (y))
#define QM_MAX(x, y) ((x) > (y) ? (x) : (y))
#define QM_ABS(x) ((x) > 0 ? (x) : -(x))

QM_INLINE float QM_PREFIX(rad_to_deg)(float rad)
{
	return rad * 57.2957795131f;
}

QM_INLINE float QM_PREFIX(deg_to_rad)(float deg)
{
	return deg * 0.01745329251f;
}

#if QM_USE_SSE

QM_INLINE __m128 QM_PREFIX(mat4_mult_column_sse)(__m128 c1, QMmat4 m2)
{
	__m128 result;

	result =                    _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(0, 0, 0, 0)), m2.packed[0]);
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(1, 1, 1, 1)), m2.packed[1]));
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(2, 2, 2, 2)), m2.packed[2]));
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(3, 3, 3, 3)), m2.packed[3]));

	return result;
}

#endif

//----------------------------------------------------------------------//
//VECTOR FUNCTIONS:

//addition:

QM_INLINE QMvec2 QM_PREFIX(vec2_add)(QMvec2 v1, QMvec2 v2)
{
	QMvec2 result;

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_add)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_add)(QMvec4 v1, QMvec4 v2)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = _mm_add_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;
	result.w = v1.w + v2.w;

	#endif

	return result;
}

//subtraction:

QM_INLINE QMvec2 QM_PREFIX(vec2_sub)(QMvec2 v1, QMvec2 v2)
{
	QMvec2 result;

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_sub)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_sub)(QMvec4 v1, QMvec4 v2)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = _mm_sub_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;
	result.w = v1.w - v2.w;

	#endif

	return result;
}

//multiplication:

QM_INLINE QMvec2 QM_PREFIX(vec2_mult)(QMvec2 v1, QMvec2 v2)
{
	QMvec2 result;

	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_mult)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;
	result.z = v1.z * v2.z;

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_mult)(QMvec4 v1, QMvec4 v2)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = _mm_mul_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;
	result.z = v1.z * v2.z;
	result.w = v1.w * v2.w;

	#endif

	return result;
}

//division:

QM_INLINE QMvec2 QM_PREFIX(vec2_div)(QMvec2 v1, QMvec2 v2)
{
	QMvec2 result;

	result.x = v1.x / v2.x;
	result.y = v1.y / v2.y;

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_div)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = v1.x / v2.x;
	result.y = v1.y / v2.y;
	result.z = v1.z / v2.z;

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_div)(QMvec4 v1, QMvec4 v2)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = _mm_div_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x / v2.x;
	result.y = v1.y / v2.y;
	result.z = v1.z / v2.z;
	result.w = v1.w / v2.w;

	#endif

	return result;
}

//scalar multiplication:

QM_INLINE QMvec2 QM_PREFIX(vec2_scale)(QMvec2 v, float s)
{
	QMvec2 result;

	result.x = v.x * s;
	result.y = v.y * s;

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_scale)(QMvec3 v, float s)
{
	QMvec3 result;

	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_scale)(QMvec4 v, float s)
{
	QMvec4 result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_mul_ps(v.packed, scale);

	#else

	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	result.w = v.w * s;

	#endif

	return result;
}

//dot product:

QM_INLINE float QM_PREFIX(vec2_dot)(QMvec2 v1, QMvec2 v2)
{
	float result;

	result = v1.x * v2.x + v1.y * v2.y;

	return result;
}

QM_INLINE float QM_PREFIX(vec3_dot)(QMvec3 v1, QMvec3 v2)
{
	float result;

	result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

	return result;
}

QM_INLINE float QM_PREFIX(vec4_dot)(QMvec4 v1, QMvec4 v2)
{
	float result;

	#if QM_USE_SSE

	__m128 r = _mm_mul_ps(v1.packed, v2.packed);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	_mm_store_ss(&result, r);

	#else

	result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;

	#endif

	return result;
}

//cross product

QM_INLINE QMvec3 QM_PREFIX(vec3_cross)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = (v1.y * v2.z) - (v1.z * v2.y);
	result.y = (v1.z * v2.x) - (v1.x * v2.z);
	result.z = (v1.x * v2.y) - (v1.y * v2.x);

	return result;
}

//length:

QM_INLINE float QM_PREFIX(vec2_length)(QMvec2 v)
{
	float result;

	result = QM_SQRTF(QM_PREFIX(vec2_dot)(v, v));

	return result;
}

QM_INLINE float QM_PREFIX(vec3_length)(QMvec3 v)
{
	float result;

	result = QM_SQRTF(QM_PREFIX(vec3_dot)(v, v));

	return result;
}

QM_INLINE float QM_PREFIX(vec4_length)(QMvec4 v)
{
	float result;

	result = QM_SQRTF(QM_PREFIX(vec4_dot)(v, v));

	return result;
}

//normalize:

QM_INLINE QMvec2 QM_PREFIX(vec2_normalize)(QMvec2 v)
{
	QMvec2 result = {0};

	float invLen = QM_PREFIX(vec2_length)(v);
	if(invLen != 0.0f)
	{
		invLen = 1.0f / invLen;
		result.x = v.x * invLen;
		result.y = v.y * invLen;
	}

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_normalize)(QMvec3 v)
{
	QMvec3 result = {0};

	float invLen = QM_PREFIX(vec3_length)(v);
	if(invLen != 0.0f)
	{
		invLen = 1.0f / invLen;
		result.x = v.x * invLen;
		result.y = v.y * invLen;
		result.z = v.z * invLen;
	}

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_normalize)(QMvec4 v)
{
	QMvec4 result = {0};

	float len = QM_PREFIX(vec4_length)(v);
	if(len != 0.0f)
	{
		#if QM_USE_SSE

		__m128 scale = _mm_set1_ps(len);
		result.packed = _mm_div_ps(v.packed, scale);

		#else

		float invLen = 1.0f / len;

		result.x = v.x * invLen;
		result.y = v.y * invLen;
		result.z = v.z * invLen;
		result.w = v.w * invLen;

		#endif
	}

	return result;
}

//distance:

QM_INLINE float QM_PREFIX(vec2_distance)(QMvec2 v1, QMvec2 v2)
{
	float result;

	QMvec2 to = QM_PREFIX(vec2_sub)(v1, v2);
	result = QM_PREFIX(vec2_length)(to);

	return result;
}

QM_INLINE float QM_PREFIX(vec3_distance)(QMvec3 v1, QMvec3 v2)
{
	float result;

	QMvec3 to = QM_PREFIX(vec3_sub)(v1, v2);
	result = QM_PREFIX(vec3_length)(to);

	return result;
}

QM_INLINE float QM_PREFIX(vec4_distance)(QMvec4 v1, QMvec4 v2)
{
	float result;

	QMvec4 to = QM_PREFIX(vec4_sub)(v1, v2);
	result = QM_PREFIX(vec4_length)(to);

	return result;
}

//equality:

QM_INLINE QMbool QM_PREFIX(vec2_equals)(QMvec2 v1, QMvec2 v2)
{
	QMbool result;

	result = (v1.x == v2.x) && (v1.y == v2.y); 

	return result;	
}

QM_INLINE QMbool QM_PREFIX(vec3_equals)(QMvec3 v1, QMvec3 v2)
{
	QMbool result;

	result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z); 

	return result;	
}

QM_INLINE QMbool QM_PREFIX(vec4_equals)(QMvec4 v1, QMvec4 v2)
{
	QMbool result;

	//TODO: there are SIMD instructions for floating point equality, find a way to get a single bool from them
	result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w); 

	return result;	
}

//min:

QM_INLINE QMvec2 QM_PREFIX(vec2_min)(QMvec2 v1, QMvec2 v2)
{
	QMvec2 result;

	result.x = QM_MIN(v1.x, v2.x);
	result.y = QM_MIN(v1.y, v2.y);

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_min)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = QM_MIN(v1.x, v2.x);
	result.y = QM_MIN(v1.y, v2.y);
	result.z = QM_MIN(v1.z, v2.z);

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_min)(QMvec4 v1, QMvec4 v2)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = _mm_min_ps(v1.packed, v2.packed);

	#else

	result.x = QM_MIN(v1.x, v2.x);
	result.y = QM_MIN(v1.y, v2.y);
	result.z = QM_MIN(v1.z, v2.z);
	result.w = QM_MIN(v1.w, v2.w);

	#endif

	return result;
}

//max:

QM_INLINE QMvec2 QM_PREFIX(vec2_max)(QMvec2 v1, QMvec2 v2)
{
	QMvec2 result;

	result.x = QM_MAX(v1.x, v2.x);
	result.y = QM_MAX(v1.y, v2.y);

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(vec3_max)(QMvec3 v1, QMvec3 v2)
{
	QMvec3 result;

	result.x = QM_MAX(v1.x, v2.x);
	result.y = QM_MAX(v1.y, v2.y);
	result.z = QM_MAX(v1.z, v2.z);

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(vec4_max)(QMvec4 v1, QMvec4 v2)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = _mm_max_ps(v1.packed, v2.packed);

	#else

	result.x = QM_MAX(v1.x, v2.x);
	result.y = QM_MAX(v1.y, v2.y);
	result.z = QM_MAX(v1.z, v2.z);
	result.w = QM_MAX(v1.w, v2.w);

	#endif

	return result;
}

//----------------------------------------------------------------------//
//MATRIX FUNCTIONS:

//initialization:

QM_INLINE QMmat3 QM_PREFIX(mat3_identity)()
{
	QMmat3 result = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_identity)()
{
	QMmat4 result = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return result;
}

//addition:

QM_INLINE QMmat3 QM_PREFIX(mat3_add)(QMmat3 m1, QMmat3 m2)
{
	QMmat3 result;

	result.m[0][0] = m1.m[0][0] + m2.m[0][0];
	result.m[0][1] = m1.m[0][1] + m2.m[0][1];
	result.m[0][2] = m1.m[0][2] + m2.m[0][2];
	result.m[1][0] = m1.m[1][0] + m2.m[1][0];
	result.m[1][1] = m1.m[1][1] + m2.m[1][1];
	result.m[1][2] = m1.m[1][2] + m2.m[1][2];
	result.m[2][0] = m1.m[2][0] + m2.m[2][0];
	result.m[2][1] = m1.m[2][1] + m2.m[2][1];
	result.m[2][2] = m1.m[2][2] + m2.m[2][2];

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_add)(QMmat4 m1, QMmat4 m2)
{
	QMmat4 result;

	#if QM_USE_SSE

	result.packed[0] = _mm_add_ps(m1.packed[0], m2.packed[0]);
	result.packed[1] = _mm_add_ps(m1.packed[1], m2.packed[1]);
	result.packed[2] = _mm_add_ps(m1.packed[2], m2.packed[2]);
	result.packed[3] = _mm_add_ps(m1.packed[3], m2.packed[3]);

	#else

	result.m[0][0] = m1.m[0][0] + m2.m[0][0];
	result.m[0][1] = m1.m[0][1] + m2.m[0][1];
	result.m[0][2] = m1.m[0][2] + m2.m[0][2];
	result.m[0][3] = m1.m[0][3] + m2.m[0][3];
	result.m[1][0] = m1.m[1][0] + m2.m[1][0];
	result.m[1][1] = m1.m[1][1] + m2.m[1][1];
	result.m[1][2] = m1.m[1][2] + m2.m[1][2];
	result.m[1][3] = m1.m[1][3] + m2.m[1][3];
	result.m[2][0] = m1.m[2][0] + m2.m[2][0];
	result.m[2][1] = m1.m[2][1] + m2.m[2][1];
	result.m[2][2] = m1.m[2][2] + m2.m[2][2];
	result.m[2][3] = m1.m[2][3] + m2.m[2][3];
	result.m[3][0] = m1.m[3][0] + m2.m[3][0];
	result.m[3][1] = m1.m[3][1] + m2.m[3][1];
	result.m[3][2] = m1.m[3][2] + m2.m[3][2];
	result.m[3][3] = m1.m[3][3] + m2.m[3][3];

	#endif

	return result;
}

//subtraction:

QM_INLINE QMmat3 QM_PREFIX(mat3_sub)(QMmat3 m1, QMmat3 m2)
{
	QMmat3 result;

	result.m[0][0] = m1.m[0][0] - m2.m[0][0];
	result.m[0][1] = m1.m[0][1] - m2.m[0][1];
	result.m[0][2] = m1.m[0][2] - m2.m[0][2];
	result.m[1][0] = m1.m[1][0] - m2.m[1][0];
	result.m[1][1] = m1.m[1][1] - m2.m[1][1];
	result.m[1][2] = m1.m[1][2] - m2.m[1][2];
	result.m[2][0] = m1.m[2][0] - m2.m[2][0];
	result.m[2][1] = m1.m[2][1] - m2.m[2][1];
	result.m[2][2] = m1.m[2][2] - m2.m[2][2];

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_sub)(QMmat4 m1, QMmat4 m2)
{
	QMmat4 result;

	#if QM_USE_SSE

	result.packed[0] = _mm_sub_ps(m1.packed[0], m2.packed[0]);
	result.packed[1] = _mm_sub_ps(m1.packed[1], m2.packed[1]);
	result.packed[2] = _mm_sub_ps(m1.packed[2], m2.packed[2]);
	result.packed[3] = _mm_sub_ps(m1.packed[3], m2.packed[3]);

	#else

	result.m[0][0] = m1.m[0][0] - m2.m[0][0];
	result.m[0][1] = m1.m[0][1] - m2.m[0][1];
	result.m[0][2] = m1.m[0][2] - m2.m[0][2];
	result.m[0][3] = m1.m[0][3] - m2.m[0][3];
	result.m[1][0] = m1.m[1][0] - m2.m[1][0];
	result.m[1][1] = m1.m[1][1] - m2.m[1][1];
	result.m[1][2] = m1.m[1][2] - m2.m[1][2];
	result.m[1][3] = m1.m[1][3] - m2.m[1][3];
	result.m[2][0] = m1.m[2][0] - m2.m[2][0];
	result.m[2][1] = m1.m[2][1] - m2.m[2][1];
	result.m[2][2] = m1.m[2][2] - m2.m[2][2];
	result.m[2][3] = m1.m[2][3] - m2.m[2][3];
	result.m[3][0] = m1.m[3][0] - m2.m[3][0];
	result.m[3][1] = m1.m[3][1] - m2.m[3][1];
	result.m[3][2] = m1.m[3][2] - m2.m[3][2];
	result.m[3][3] = m1.m[3][3] - m2.m[3][3];

	#endif

	return result;
}

//multiplication:

QM_INLINE QMmat3 QM_PREFIX(mat3_mult)(QMmat3 m1, QMmat3 m2)
{
	QMmat3 result;

	result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2];
	result.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2];
	result.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2];
	result.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2];
	result.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2];
	result.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2];
	result.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2];
	result.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2];
	result.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2];

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_mult)(QMmat4 m1, QMmat4 m2)
{
	QMmat4 result;

	#if QM_USE_SSE

	result.packed[0] = QM_PREFIX(mat4_mult_column_sse)(m2.packed[0], m1);
	result.packed[1] = QM_PREFIX(mat4_mult_column_sse)(m2.packed[1], m1);
	result.packed[2] = QM_PREFIX(mat4_mult_column_sse)(m2.packed[2], m1);
	result.packed[3] = QM_PREFIX(mat4_mult_column_sse)(m2.packed[3], m1);

	#else

	result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2] + m1.m[3][0] * m2.m[0][3];
	result.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2] + m1.m[3][1] * m2.m[0][3];
	result.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2] + m1.m[3][2] * m2.m[0][3];
	result.m[0][3] = m1.m[0][3] * m2.m[0][0] + m1.m[1][3] * m2.m[0][1] + m1.m[2][3] * m2.m[0][2] + m1.m[3][3] * m2.m[0][3];
	result.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2] + m1.m[3][0] * m2.m[1][3];
	result.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2] + m1.m[3][1] * m2.m[1][3];
	result.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2] + m1.m[3][2] * m2.m[1][3];
	result.m[1][3] = m1.m[0][3] * m2.m[1][0] + m1.m[1][3] * m2.m[1][1] + m1.m[2][3] * m2.m[1][2] + m1.m[3][3] * m2.m[1][3];
	result.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2] + m1.m[3][0] * m2.m[2][3];
	result.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2] + m1.m[3][1] * m2.m[2][3];
	result.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2] + m1.m[3][2] * m2.m[2][3];
	result.m[2][3] = m1.m[0][3] * m2.m[2][0] + m1.m[1][3] * m2.m[2][1] + m1.m[2][3] * m2.m[2][2] + m1.m[3][3] * m2.m[2][3];
	result.m[3][0] = m1.m[0][0] * m2.m[3][0] + m1.m[1][0] * m2.m[3][1] + m1.m[2][0] * m2.m[3][2] + m1.m[3][0] * m2.m[3][3];
	result.m[3][1] = m1.m[0][1] * m2.m[3][0] + m1.m[1][1] * m2.m[3][1] + m1.m[2][1] * m2.m[3][2] + m1.m[3][1] * m2.m[3][3];
	result.m[3][2] = m1.m[0][2] * m2.m[3][0] + m1.m[1][2] * m2.m[3][1] + m1.m[2][2] * m2.m[3][2] + m1.m[3][2] * m2.m[3][3];
	result.m[3][3] = m1.m[0][3] * m2.m[3][0] + m1.m[1][3] * m2.m[3][1] + m1.m[2][3] * m2.m[3][2] + m1.m[3][3] * m2.m[3][3];

	#endif

	return result;
}

QM_INLINE QMvec3 QM_PREFIX(mat3_mult_vec3)(QMmat3 m, QMvec3 v)
{
	QMvec3 result;

	result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z;
	result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z;
	result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z;

	return result;
}

QM_INLINE QMvec4 QM_PREFIX(mat4_mult_vec4)(QMmat4 m, QMvec4 v)
{
	QMvec4 result;

	#if QM_USE_SSE

	result.packed = QM_PREFIX(mat4_mult_column_sse)(v.packed, m);

	#else

	result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;
	result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;
	result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;
	result.w = m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

	#endif

	return result;
}

//transpose:

QM_INLINE QMmat3 QM_PREFIX(mat3_transpose)(QMmat3 m)
{
	QMmat3 result;

	result.m[0][0] = m.m[0][0];
	result.m[0][1] = m.m[1][0];
	result.m[0][2] = m.m[2][0];
	result.m[1][0] = m.m[0][1];
	result.m[1][1] = m.m[1][1];
	result.m[1][2] = m.m[2][1];
	result.m[2][0] = m.m[0][2];
	result.m[2][1] = m.m[1][2];
	result.m[2][2] = m.m[2][2];

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_transpose)(QMmat4 m)
{
	QMmat4 result = m;

	#if QM_USE_SSE

	_MM_TRANSPOSE4_PS(result.packed[0], result.packed[1], result.packed[2], result.packed[3]);

	#else

	result.m[0][0] = m.m[0][0];
	result.m[0][1] = m.m[1][0];
	result.m[0][2] = m.m[2][0];
	result.m[0][3] = m.m[3][0];
	result.m[1][0] = m.m[0][1];
	result.m[1][1] = m.m[1][1];
	result.m[1][2] = m.m[2][1];
	result.m[1][3] = m.m[3][1];
	result.m[2][0] = m.m[0][2];
	result.m[2][1] = m.m[1][2];
	result.m[2][2] = m.m[2][2];
	result.m[2][3] = m.m[3][2];
	result.m[3][0] = m.m[0][3];
	result.m[3][1] = m.m[1][3];
	result.m[3][2] = m.m[2][3];
	result.m[3][3] = m.m[3][3];

	#endif

	return result;
}

//inverse:

QM_INLINE QMmat3 QM_PREFIX(mat3_inv)(QMmat3 m)
{
	QMmat3 result;

	float det;
  	float a = m.m[0][0], b = m.m[0][1], c = m.m[0][2],
	      d = m.m[1][0], e = m.m[1][1], f = m.m[1][2],
	      g = m.m[2][0], h = m.m[2][1], i = m.m[2][2];

	result.m[0][0] =   e * i - f * h;
	result.m[0][1] = -(b * i - h * c);
	result.m[0][2] =   b * f - e * c;
	result.m[1][0] = -(d * i - g * f);
	result.m[1][1] =   a * i - c * g;
	result.m[1][2] = -(a * f - d * c);
	result.m[2][0] =   d * h - g * e;
	result.m[2][1] = -(a * h - g * b);
	result.m[2][2] =   a * e - b * d;

	det = 1.0f / (a * result.m[0][0] + b * result.m[1][0] + c * result.m[2][0]);

	result.m[0][0] *= det;
	result.m[0][1] *= det;
	result.m[0][2] *= det;
	result.m[1][0] *= det;
	result.m[1][1] *= det;
	result.m[1][2] *= det;
	result.m[2][0] *= det;
	result.m[2][1] *= det;
	result.m[2][2] *= det;

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_inv)(QMmat4 mat)
{
	//TODO: this function is not SIMD optimized, figure out how to do it

	QMmat4 result;

	float tmp[6];
	float det;
	float a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2], d = mat.m[0][3],
	      e = mat.m[1][0], f = mat.m[1][1], g = mat.m[1][2], h = mat.m[1][3],
	      i = mat.m[2][0], j = mat.m[2][1], k = mat.m[2][2], l = mat.m[2][3],
	      m = mat.m[3][0], n = mat.m[3][1], o = mat.m[3][2], p = mat.m[3][3];

	tmp[0] = k * p - o * l; 
	tmp[1] = j * p - n * l; 
	tmp[2] = j * o - n * k;
	tmp[3] = i * p - m * l; 
	tmp[4] = i * o - m * k; 
	tmp[5] = i * n - m * j;

	result.m[0][0] =   f * tmp[0] - g * tmp[1] + h * tmp[2];
	result.m[1][0] = -(e * tmp[0] - g * tmp[3] + h * tmp[4]);
	result.m[2][0] =   e * tmp[1] - f * tmp[3] + h * tmp[5];
	result.m[3][0] = -(e * tmp[2] - f * tmp[4] + g * tmp[5]);

	result.m[0][1] = -(b * tmp[0] - c * tmp[1] + d * tmp[2]);
	result.m[1][1] =   a * tmp[0] - c * tmp[3] + d * tmp[4];
	result.m[2][1] = -(a * tmp[1] - b * tmp[3] + d * tmp[5]);
	result.m[3][1] =   a * tmp[2] - b * tmp[4] + c * tmp[5];

	tmp[0] = g * p - o * h;
	tmp[1] = f * p - n * h;
	tmp[2] = f * o - n * g;
	tmp[3] = e * p - m * h;
	tmp[4] = e * o - m * g;
	tmp[5] = e * n - m * f;

	result.m[0][2] =   b * tmp[0] - c * tmp[1] + d * tmp[2];
	result.m[1][2] = -(a * tmp[0] - c * tmp[3] + d * tmp[4]);
	result.m[2][2] =   a * tmp[1] - b * tmp[3] + d * tmp[5];
	result.m[3][2] = -(a * tmp[2] - b * tmp[4] + c * tmp[5]);

	tmp[0] = g * l - k * h;
	tmp[1] = f * l - j * h;
	tmp[2] = f * k - j * g;
	tmp[3] = e * l - i * h;
	tmp[4] = e * k - i * g;
	tmp[5] = e * j - i * f;

	result.m[0][3] = -(b * tmp[0] - c * tmp[1] + d * tmp[2]);
	result.m[1][3] =   a * tmp[0] - c * tmp[3] + d * tmp[4];
	result.m[2][3] = -(a * tmp[1] - b * tmp[3] + d * tmp[5]);
  	result.m[3][3] =   a * tmp[2] - b * tmp[4] + c * tmp[5];

  	det = 1.0f / (a * result.m[0][0] + b * result.m[1][0]
                + c * result.m[2][0] + d * result.m[3][0]);

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(det);
	result.packed[0] = _mm_mul_ps(result.packed[0], scale);
	result.packed[1] = _mm_mul_ps(result.packed[1], scale);
	result.packed[2] = _mm_mul_ps(result.packed[2], scale);
	result.packed[3] = _mm_mul_ps(result.packed[3], scale);

	#else

	result.m[0][0] = result.m[0][0] * det;
	result.m[0][1] = result.m[0][1] * det;
	result.m[0][2] = result.m[0][2] * det;
	result.m[0][3] = result.m[0][3] * det;
	result.m[1][0] = result.m[1][0] * det;
	result.m[1][1] = result.m[1][1] * det;
	result.m[1][2] = result.m[1][2] * det;
	result.m[1][3] = result.m[1][3] * det;
	result.m[2][0] = result.m[2][0] * det;
	result.m[2][1] = result.m[2][1] * det;
	result.m[2][2] = result.m[2][2] * det;
	result.m[2][3] = result.m[2][3] * det;
	result.m[3][0] = result.m[3][0] * det;
	result.m[3][1] = result.m[3][1] * det;
	result.m[3][2] = result.m[3][2] * det;
	result.m[3][3] = result.m[3][3] * det;

	#endif

  	return result;
}

//translation:

QM_INLINE QMmat3 QM_PREFIX(mat3_translate)(QMvec2 t)
{
	QMmat3 result = QM_PREFIX(mat3_identity)();

	result.m[2][0] = t.x;
	result.m[2][1] = t.y;

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_translate)(QMvec3 t)
{
	QMmat4 result = QM_PREFIX(mat4_identity)();

	result.m[3][0] = t.x;
	result.m[3][1] = t.y;
	result.m[3][2] = t.z;

	return result;
}

//scaling:

QM_INLINE QMmat3 QM_PREFIX(mat3_scale)(QMvec2 s)
{
	QMmat3 result = QM_PREFIX(mat3_identity)();

	result.m[0][0] = s.x;
	result.m[1][1] = s.y;

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_scale)(QMvec3 s)
{
	QMmat4 result = QM_PREFIX(mat4_identity)();

	result.m[0][0] = s.x;
	result.m[1][1] = s.y;
	result.m[2][2] = s.z;

	return result;
}

//rotation:

QM_INLINE QMmat3 QM_PREFIX(mat3_rotate)(float angle)
{
	QMmat3 result = QM_PREFIX(mat3_identity)();

	float radians = QM_PREFIX(deg_to_rad)(angle);
	float sine   = QM_SINF(radians);
	float cosine = QM_COSF(radians);

	result.m[0][0] = cosine;
	result.m[1][0] =   sine;
	result.m[0][1] =  -sine;
	result.m[1][1] = cosine;

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_rotate)(QMvec3 axis, float angle)
{
	QMmat4 result = QM_PREFIX(mat4_identity)();

	axis = QM_PREFIX(vec3_normalize)(axis);

	float radians = QM_PREFIX(deg_to_rad)(angle);
	float sine    = QM_SINF(radians);
	float cosine  = QM_COSF(radians);
	float cosine2 = 1.0f - cosine;

	result.m[0][0] = axis.x * axis.x * cosine2 + cosine;
	result.m[0][1] = axis.x * axis.y * cosine2 + axis.z * sine;
	result.m[0][2] = axis.x * axis.z * cosine2 - axis.y * sine;
	result.m[1][0] = axis.y * axis.x * cosine2 - axis.z * sine;
	result.m[1][1] = axis.y * axis.y * cosine2 + cosine;
	result.m[1][2] = axis.y * axis.z * cosine2 + axis.x * sine;
	result.m[2][0] = axis.z * axis.x * cosine2 + axis.y * sine;
	result.m[2][1] = axis.z * axis.y * cosine2 - axis.x * sine;
	result.m[2][2] = axis.z * axis.z * cosine2 + cosine;

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_rotate_euler)(QMvec3 angles)
{
	QMmat4 result = QM_PREFIX(mat4_identity)();

	QMvec3 radians;
	radians.x = QM_PREFIX(deg_to_rad)(angles.x);
	radians.y = QM_PREFIX(deg_to_rad)(angles.y);
	radians.z = QM_PREFIX(deg_to_rad)(angles.z);

	float sinX = QM_SINF(radians.x);
	float cosX = QM_COSF(radians.x);
	float sinY = QM_SINF(radians.y);
	float cosY = QM_COSF(radians.y);
	float sinZ = QM_SINF(radians.z);
	float cosZ = QM_COSF(radians.z);

	result.m[0][0] = cosY * cosZ;
	result.m[0][1] = cosY * sinZ;
	result.m[0][2] = -sinY;
	result.m[1][0] = sinX * sinY * cosZ - cosX * sinZ;
	result.m[1][1] = sinX * sinY * sinZ + cosX * cosZ;
	result.m[1][2] = sinX * cosY;
	result.m[2][0] = cosX * sinY * cosZ + sinX * sinZ;
	result.m[2][1] = cosX * sinY * sinZ - sinX * cosZ;
	result.m[2][2] = cosX * cosY;

	return result;
}

//to mat3:

QM_INLINE QMmat3 QM_PREFIX(mat4_top_left)(QMmat4 m)
{
	QMmat3 result;

	result.m[0][0] = m.m[0][0];
	result.m[0][1] = m.m[0][1];
	result.m[0][2] = m.m[0][2];
	result.m[1][0] = m.m[1][0];
	result.m[1][1] = m.m[1][1];
	result.m[1][2] = m.m[1][2];
	result.m[2][0] = m.m[2][0];
	result.m[2][1] = m.m[2][1];
	result.m[2][2] = m.m[2][2];

	return result;
}

//projection:

QM_INLINE QMmat4 QM_PREFIX(mat4_perspective)(float fov, float aspect, float near, float far)
{
	QMmat4 result = {0};

	float scale = QM_TANF(QM_PREFIX(deg_to_rad)(fov * 0.5f)) * near;

	float right = aspect * scale;
	float left  = -right;
	float top   = scale;
	float bot   = -top;

	result.m[0][0] = near / right;
	result.m[1][1] = near / top;
	result.m[2][2] = -(far + near) / (far - near);
	result.m[3][2] = -2.0f * far * near / (far - near);
	result.m[2][3] = -1.0f;

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_orthographic)(float left, float right, float bot, float top, float near, float far)
{
	QMmat4 result = QM_PREFIX(mat4_identity)();

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bot);
	result.m[2][2] = 2.0f / (near - far);

	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (bot  + top  ) / (bot  - top  );
	result.m[3][2] = (near + far  ) / (near - far  );

	return result;
}

//view matrix:

QM_INLINE QMmat4 QM_PREFIX(mat4_look)(QMvec3 pos, QMvec3 dir, QMvec3 up)
{
	QMmat4 result;

	QMvec3 r = QM_PREFIX(vec3_normalize)(QM_PREFIX(vec3_cross)(up, dir));
	QMvec3 u = QM_PREFIX(vec3_cross)(dir, r);

	QMmat4 RUD = QM_PREFIX(mat4_identity)();
	RUD.m[0][0] = r.x;
	RUD.m[1][0] = r.y;
	RUD.m[2][0] = r.z;
	RUD.m[0][1] = u.x;
	RUD.m[1][1] = u.y;
	RUD.m[2][1] = u.z;
	RUD.m[0][2] = dir.x;
	RUD.m[1][2] = dir.y;
	RUD.m[2][2] = dir.z;

	QMvec3 oppPos = {-pos.x, -pos.y, -pos.z};	
	result = QM_PREFIX(mat4_mult)(RUD, QM_PREFIX(mat4_translate)(oppPos));

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(mat4_lookat)(QMvec3 pos, QMvec3 target, QMvec3 up)
{
	QMmat4 result;

	QMvec3 dir = QM_PREFIX(vec3_normalize)(QM_PREFIX(vec3_sub)(pos, target));
	result = QM_PREFIX(mat4_look)(pos, dir, up);

	return result;
}

//----------------------------------------------------------------------//
//QUATERNION FUNCTIONS:

QM_INLINE QMquaternion QM_PREFIX(quaternion_identity)()
{
	QMquaternion result;

	result.x = 0.0f;
	result.y = 0.0f;
	result.z = 0.0f;
	result.w = 1.0f;

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_add)(QMquaternion q1, QMquaternion q2)
{
	QMquaternion result;

	#if QM_USE_SSE

	result.packed = _mm_add_ps(q1.packed, q2.packed);

	#else

	result.x = q1.x + q2.x;
	result.y = q1.y + q2.y;
	result.z = q1.z + q2.z;
	result.w = q1.w + q2.w;

	#endif

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_sub)(QMquaternion q1, QMquaternion q2)
{
	QMquaternion result;

	#if QM_USE_SSE

	result.packed = _mm_sub_ps(q1.packed, q2.packed);

	#else

	result.x = q1.x - q2.x;
	result.y = q1.y - q2.y;
	result.z = q1.z - q2.z;
	result.w = q1.w - q2.w;

	#endif

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_mult)(QMquaternion q1, QMquaternion q2)
{
	QMquaternion result;

	#if QM_USE_SSE

	__m128 temp1;
	__m128 temp2;

	temp1 = _mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(3, 3, 3, 3));
	temp2 = q2.packed;
	result.packed = _mm_mul_ps(temp1, temp2);

	temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f));
	temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(0, 1, 2, 3));
	result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

	temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f));
	temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(1, 0, 3, 2));
	result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

	temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.0f, 0.0f, 0.0f, -0.0f));
	temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(2, 3, 0, 1));
	result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

	#else

	result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

	#endif

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_scale)(QMquaternion q, float s)
{
	QMquaternion result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_mul_ps(q.packed, scale);

	#else

	result.x = q.x * s;
	result.y = q.y * s;
	result.z = q.z * s;
	result.w = q.w * s;

	#endif

	return result;
}

QM_INLINE float QM_PREFIX(quaternion_dot)(QMquaternion q1, QMquaternion q2)
{
	float result;

	#if QM_USE_SSE

	__m128 r = _mm_mul_ps(q1.packed, q2.packed);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	_mm_store_ss(&result, r);

	#else

	result = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

	#endif

	return result;
}

QM_INLINE float QM_PREFIX(quaternion_length)(QMquaternion q)
{
	float result;

	result = QM_SQRTF(QM_PREFIX(quaternion_dot)(q, q));

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_normalize)(QMquaternion q)
{
	QMquaternion result = {0};

	float len = QM_PREFIX(quaternion_length)(q);
	if(len != 0.0f)
	{
		#if QM_USE_SSE

		__m128 scale = _mm_set1_ps(len);
		result.packed = _mm_div_ps(q.packed, scale);

		#else

		float invLen = 1.0f / len;

		result.x = q.x * invLen;
		result.y = q.y * invLen;
		result.z = q.z * invLen;
		result.w = q.w * invLen;

		#endif
	}

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_conjugate)(QMquaternion q)
{
	QMquaternion result;

	result.x = -q.x;
	result.y = -q.y;
	result.z = -q.z;
	result.w = q.w;

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_inv)(QMquaternion q)
{
	QMquaternion result;

	result.x = -q.x;
	result.y = -q.y;
	result.z = -q.z;
	result.w = q.w;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(QM_PREFIX(quaternion_dot)(q, q));
	_mm_div_ps(result.packed, scale);

	#else

	float invLen2 = 1.0f / QM_PREFIX(quaternion_dot)(q, q);

	result.x *= invLen2;
	result.y *= invLen2;
	result.z *= invLen2;
	result.w *= invLen2;

	#endif

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_slerp)(QMquaternion q1, QMquaternion q2, float a)
{
	QMquaternion result;

	float cosine = QM_PREFIX(quaternion_dot)(q1, q2);
	float angle = QM_ACOSF(cosine);

	float sine1 = QM_SINF((1.0f - a) * angle);
	float sine2 = QM_SINF(a * angle);
	float invSine = 1.0f / QM_SINF(angle);

	q1 = QM_PREFIX(quaternion_scale)(q1, sine1);
	q2 = QM_PREFIX(quaternion_scale)(q2, sine2);

	result = QM_PREFIX(quaternion_add)(q1, q2);
	result = QM_PREFIX(quaternion_scale)(result, invSine);

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_from_axis_angle)(QMvec3 axis, float angle)
{
	QMquaternion result;

	float radians = QM_PREFIX(deg_to_rad)(angle * 0.5f);
	axis = QM_PREFIX(vec3_normalize)(axis);
	float sine = QM_SINF(radians);

	result.x = axis.x * sine;
	result.y = axis.y * sine;
	result.z = axis.z * sine;
	result.w = QM_COSF(radians);

	return result;
}

QM_INLINE QMquaternion QM_PREFIX(quaternion_from_euler)(QMvec3 angles)
{
	QMquaternion result;

	QMvec3 radians;
	radians.x = QM_PREFIX(deg_to_rad)(angles.x * 0.5f);
	radians.y = QM_PREFIX(deg_to_rad)(angles.y * 0.5f);
	radians.z = QM_PREFIX(deg_to_rad)(angles.z * 0.5f);

	float sinx = QM_SINF(radians.x);
	float cosx = QM_COSF(radians.x);
	float siny = QM_SINF(radians.y);
	float cosy = QM_COSF(radians.y);
	float sinz = QM_SINF(radians.z);
	float cosz = QM_COSF(radians.z);

	#if QM_USE_SSE

	__m128 packedx = _mm_setr_ps(sinx, cosx, cosx, cosx);
	__m128 packedy = _mm_setr_ps(cosy, siny, cosy, cosy);
	__m128 packedz = _mm_setr_ps(cosz, cosz, sinz, cosz);

	result.packed = _mm_mul_ps(_mm_mul_ps(packedx, packedy), packedz);

	packedx = _mm_shuffle_ps(packedx, packedx, _MM_SHUFFLE(0, 0, 0, 1));
	packedy = _mm_shuffle_ps(packedy, packedy, _MM_SHUFFLE(1, 1, 0, 1));
	packedz = _mm_shuffle_ps(packedz, packedz, _MM_SHUFFLE(2, 0, 2, 2));

	result.packed = _mm_addsub_ps(result.packed, _mm_mul_ps(_mm_mul_ps(packedx, packedy), packedz));

	#else

	result.x = sinx * cosy * cosz - cosx * siny * sinz;
	result.y = cosx * siny * cosz + sinx * cosy * sinz;
	result.z = cosx * cosy * sinz - sinx * siny * cosz;
	result.w = cosx * cosy * cosz + sinx * siny * sinz;

	#endif

	return result;
}

QM_INLINE QMmat4 QM_PREFIX(quaternion_to_mat4)(QMquaternion q)
{
	QMmat4 result = QM_PREFIX(mat4_identity)();

	float x2  = q.x + q.x;
    float y2  = q.y + q.y;
    float z2  = q.z + q.z;
    float xx2 = q.x * x2;
    float xy2 = q.x * y2;
    float xz2 = q.x * z2;
    float yy2 = q.y * y2;
    float yz2 = q.y * z2;
    float zz2 = q.z * z2;
    float sx2 = q.w * x2;
    float sy2 = q.w * y2;
    float sz2 = q.w * z2;

	result.m[0][0] = 1.0f - (yy2 + zz2);
	result.m[0][1] = xy2 - sz2;
	result.m[0][2] = xz2 + sy2;
	result.m[1][0] = xy2 + sz2;
	result.m[1][1] = 1.0f - (xx2 + zz2);
	result.m[1][2] = yz2 - sx2;
	result.m[2][0] = xz2 - sy2;
	result.m[2][1] = yz2 + sx2;
	result.m[2][2] = 1.0f - (xx2 + yy2);

	return result;
}

#ifdef __cplusplus
} //extern "C"
#endif

#endif //QM_MATH_H
