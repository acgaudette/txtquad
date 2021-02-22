#ifndef ALG_H
#define ALG_H

#define ALG_INLINE __attribute__((__always_inline__))

#define _USE_MATH_DEFINES
#include <math.h>
#include "nmmintrin.h"
#ifdef ALG_DEBUG
#include <assert.h>
#endif
#include "types.h"

#define VEC(N, ...) typedef union v ## N { \
        float s[N];                        \
        __VA_ARGS__;                       \
} v ## N

#define COMP_2 float x; \
               float y;
#define COMP_3 COMP_2 float z;
#define COMP_4 COMP_3 float w;

VEC(2, struct { COMP_2 });
VEC(3, struct { COMP_3 };
       struct { v2 xy; float _0; };
       struct { float _1; v2 yz; });
VEC(4, struct { COMP_4 };
       struct { v2 xy; v2 zw; };
       struct { float _0; v2 yz; float _1; };
       struct { v3 xyz; float _2; };
       struct { float _3; float yzw; };
       __m128 v);
#undef VEC

typedef v2 ff;
typedef v3 fff;
typedef v4 ffff;

#define MAT(N, ...) typedef union m ## N { \
        float s[N * N];                    \
        v ## N v[N];                       \
        struct { __VA_ARGS__ };            \
} m ## N

#define VEC_2(N) v ## N c0; \
                 v ## N c1;
#define VEC_3(N) VEC_2(N) v ## N c2;
#define VEC_4 VEC_3(4) v4 c3;

MAT(2, VEC_2(2));
MAT(3, VEC_3(3));
MAT(4, VEC_4);
#undef MAT

#undef COMP_2
#undef  VEC_2
#undef COMP_3
#undef  VEC_3
#undef COMP_4
#undef  VEC_4

/* Floating point functions */

static inline float minf(const float a, const float b)
{
	return a < b ? a : b;
}

static inline float maxf(const float a, const float b)
{
	return a < b ? b : a;
}

static inline float clampf(const float s, const float min, const float max)
{
	return minf(max, maxf(min, s));
}

static inline float clamp01f(const float s)
{
	return clampf(s, 0.f, 1.f);
}

static inline float lerpf(const float a, const float b, const float t)
{
	return (1.f - t) * a + t * b;
}

static inline float lerpf_clamp(const float a, const float b, const float t)
{
	return lerpf(a, b, clamp01f(t));
}

static float mixf(const ff range, const float t)
{
	return lerpf(range.x, range.y, t);
}

static float mixf_safe(const ff range, const float t)
{
	return lerpf_clamp(range.x, range.y, t);
}

static inline float signf(const float s)
{
	u32 u = *((u32*)&s);
	return (float)(u >> 31) * -2.f + 1.f;
}

static inline int is01f(const float s)
{
	return s == clamp01f(s);
}

/* Vectors */

#define GEN(N, ID, FV) \
	ID(N, v ## N ## _ ## ID) \
	ID(N, ID ## FV)

#define V2_FILL(S) ((v2) { S, S })
#define V3_FILL(S) ((v3) { S, S, S })
#define V4_FILL(S) ((v4) { S, S, S, S })

#define shift(N, ID) static v ## N ID(v ## N v, const u8 i) \
{ \
	v ## N swap = v; \
	for (u8 j = 0; j < N; ++j) { \
		u8 k = (i + j) % N; \
		v.s[j] = swap.s[k]; \
	} \
	return v; \
}

GEN(2, shift, ff)
GEN(3, shift, fff)
GEN(4, shift, ffff)
#undef shift

#define V2_ZERO ((v2) {})
#define V3_ZERO ((v3) {})
#define V4_ZERO ((v4) {})

#define V2_ONE V2_FILL(1.f)
#define V3_ONE V3_FILL(1.f)
#define V4_ONE V4_FILL(1.f)

#define V2_RT ((v2) { 1.f, 0.f })
#define V3_RT ((v3) { 1.f, 0.f, 0.f })
#define V4_RT ((v4) { 1.f, 0.f, 0.f, 0.f })

#define V2_UP ((v2) { 0.f, 1.f })
#define V3_UP ((v3) { 0.f, 1.f, 0.f })
#define V4_UP ((v4) { 0.f, 1.f, 0.f, 0.f })

#define V3_FWD ((v3) { 0.f, 0.f, 1.f })
#define V4_FWD ((v4) { 0.f, 0.f, 1.f, 0.f })

#define V2_LFT ((v2) { -1.f, 0.f })
#define V3_LFT ((v3) { -1.f, 0.f, 0.f })
#define V4_LFT ((v4) { -1.f, 0.f, 0.f, 0.f })

#define V2_DN ((v2) { 0.f, -1.f })
#define V3_DN ((v3) { 0.f, -1.f, 0.f })
#define V4_DN ((v4) { 0.f, -1.f, 0.f, 0.f })

#define V3_BCK ((v3) { 0.f, 0.f, -1.f })
#define V4_BCK ((v4) { 0.f, 0.f, -1.f, 0.f })

#define EQ(N) static int v ## N ## _eq(v ## N a, v ## N b) \
{ \
	int result = 1; \
	for (u8 i = 0; i < N; ++i) \
		result &= a.s[i] == b.s[i]; \
	return result; \
}

EQ(2)
EQ(3)
EQ(4)
#undef EQ

#define FZ_EQ(N) static int v ## N ## _fzeq(v ## N a, v ## N b) \
{ \
	int result = 1; \
	for (u8 i = 0; i < N; ++i) \
		result &= fabsf(a.s[i] - b.s[i]) < __FLT_EPSILON__; \
	return result; \
}

FZ_EQ(2)
FZ_EQ(3)
FZ_EQ(4)
#undef FZ_EQ

#define neg(N, ID) static v ## N ID(v ## N v) \
{ \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] *= -1.f; \
	return v; \
}

GEN(2, neg, ff)
GEN(3, neg, fff)
GEN(4, neg, ffff)
#undef neg

#define add(N, ID) static v ## N ID(v ## N a, v ## N b) \
{ \
	for (size_t i = 0; i < N; ++i) \
		a.s[i] += b.s[i]; \
	return a; \
}

GEN(2, add, ff)
GEN(3, add, fff)
GEN(4, add, ffff)
#undef add

#define addeq(N, ID) static void ID(v ## N *a, v ## N b) \
{ \
	*a = v ## N ## _add(*a, b); \
}

GEN(2, addeq, ff)
GEN(3, addeq, fff)
GEN(4, addeq, ffff)
#undef addeq

#define sub(N, ID) static v ## N ID(v ## N a, v ## N b) \
{ \
	for (size_t i = 0; i < N; ++i) \
		a.s[i] -= b.s[i]; \
	return a; \
}

GEN(2, sub, ff)
GEN(3, sub, fff)
GEN(4, sub, ffff)
#undef sub

#define mul(N, ID) static v ## N ID(v ## N v, float s) \
{ \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] *= s; \
	return v; \
}

GEN(2, mul, ff)
GEN(3, mul, fff)
GEN(4, mul, ffff)
#undef mul

#define muleq(N, ID) static void ID(v ## N *v, float s) \
{ \
	*v = v ## N ## _mul(*v, s); \
}

GEN(2, muleq, ff)
GEN(3, muleq, fff)
GEN(4, muleq, ffff)
#undef muleq

#define schur(N, ID) static v ## N ID(v ## N a, v ## N b) \
{ \
	for (size_t i = 0; i < N; ++i) \
		a.s[i] *= b.s[i]; \
	return a; \
}

GEN(2, schur, ff)
GEN(3, schur, fff)
GEN(4, schur, ffff)
#undef schur

#define magsq(N, ID) static float ID(v ## N v) \
{ \
	float s = 0; \
	for (size_t i = 0; i < N; ++i) \
		s += v.s[i] * v.s[i]; \
	return s; \
}

GEN(2, magsq, ff)
GEN(3, magsq, fff)
GEN(4, magsq, ffff)
#undef magsq

#define mag(N, ID) static float ID(v ## N v) \
{ \
	return sqrtf(v ## N ## _magsq(v)); \
}

GEN(2, mag, ff)
GEN(3, mag, fff)
GEN(4, mag, ffff)
#undef mag

#define norm(N, ID) static v ## N ID(v ## N v) \
{ \
	float mag = v ## N ## _mag(v); \
	float inv = 1.f / mag; \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] *= inv; \
	return v; \
}

GEN(2, norm, ff)
GEN(3, norm, fff)
GEN(4, norm, ffff)
#undef norm

#define isnorm(N, ID) static int ID(v ## N v) \
{ \
	float magsq = v ## N ## _magsq(v); \
	return fabsf(1.f - magsq) < 1e-6; \
}

GEN(2, isnorm, ff)
GEN(3, isnorm, fff)
GEN(4, isnorm, ffff)
#undef isnorm

#define dot(N, ID) static float ID(v ## N a, v ## N b)         \
{                                                              \
        float s = 0;                                           \
        for (size_t i = 0; i < N; ++i)                         \
                s += a.s[i] * b.s[i];                          \
        return s;                                              \
}

GEN(2, dot, ff)
#undef dot

static ALG_INLINE float v4_dot(v4 a, v4 b)
{
	v4 result = { .v = _mm_mul_ps(a.v, b.v) };
	return result.x + result.y + result.z + result.w;
}

static ALG_INLINE float v3_dot(v3 a, v3 b)
{
	const float xx = a.x * b.x;
	const float yy = a.y * b.y;
	const float zz = a.z * b.z;
	return xx + yy + zz;
}

#define dotfff (a, b) v3_dot(a, b)
#define dotffff(a, b) v4_dot(a, b)

#define LERP(N) static v ## N v ## N ## _lerp(v ## N a, v ## N b, float s) \
{ \
	v ## N v = v ## N ## _add( \
		v ## N ## _mul(a, (1.f - s)), \
		v ## N ## _mul(b, s) \
	); \
	return v; \
}

LERP(2)
LERP(3)
LERP(4)
#undef LERP

#define mixff   (a, b, t) v2_lerp(a, b, t)
#define mixfff  (a, b, t) v3_lerp(a, b, t)
#define mixffff (a, b, t) v4_lerp(a, b, t)

#define LERP_CLAMP(N) static v ## N v ## N ## _lerp_clamp(v ## N a, v ## N b, float s) \
{ \
	return v ## N ## _lerp(a, b, clamp01f(s)); \
}

LERP_CLAMP(2)
LERP_CLAMP(3)
LERP_CLAMP(4)
#undef LERP_CLAMP

#define mixff_safe(a, b, t) v2_lerp_clamp(a, b, t)
#define mixfff_safe(a, b, t) v3_lerp_clamp(a, b, t)
#define mixffff_safe(a, b, t) v4_lerp_clamp(a, b, t)

static v3 v3_cross(v3 a, v3 b)
{
	return (v3) {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

#define crossfff(a, b) v3_cross(a, b)

#undef VFN_GEN

/* Matrices */

#define M2_ZERO ((m2) {})
#define M3_ZERO ((m3) {})
#define M4_ZERO ((m4) {})

#define M2_ID ((m2) { .s[0] = 1.f, .s[3] = 1.f })
#define M3_ID ((m3) { .s[0] = 1.f, .s[4] = 1.f, .s [8] = 1.f })
#define M4_ID ((m4) { .s[0] = 1.f, .s[5] = 1.f, .s[10] = 1.f, .s[15] = 1.f })

// Note: you can retrieve the columns from the union directly
#define ROW(N) static v ## N m ## N ## _row(m ## N m, size_t r) \
{                                                               \
        v ## N v;                                               \
        for (size_t i = 0; i < N; ++i)                          \
                v.s[i] = m.v[i].s[r];                           \
        return v;                                               \
}

ROW(2)
ROW(3)
ROW(4)
#undef ROW

#define MUL(N) static m ## N m ## N ## _mul(m ## N a, m ## N b) \
{ \
	m ## N m; \
	for (size_t i = 0; i < N; ++i) { \
		m.v[i] = V ## N ## _ZERO; \
		for (size_t k = 0; k < N; ++k) { \
			for (size_t j = 0; j < N; ++j) { \
				m.v[i].s[j] += b.v[i].s[k] * a.v[k].s[j]; \
			} \
		} \
	} \
	return m; \
}

MUL(2)
MUL(4)
#undef MUL

// Hand-unroll for m4_model() bottleneck
static m3 m3_mul(m3 a, m3 b)
{
	const float x0 = a.c0.x * b.c0.x + a.c1.x * b.c0.y + a.c2.x * b.c0.z;
	const float x1 = a.c0.x * b.c1.x + a.c1.x * b.c1.y + a.c2.x * b.c1.z;
	const float x2 = a.c0.x * b.c2.x + a.c1.x * b.c2.y + a.c2.x * b.c2.z;
	const float y0 = a.c0.y * b.c0.x + a.c1.y * b.c0.y + a.c2.y * b.c0.z;
	const float y1 = a.c0.y * b.c1.x + a.c1.y * b.c1.y + a.c2.y * b.c1.z;
	const float y2 = a.c0.y * b.c2.x + a.c1.y * b.c2.y + a.c2.y * b.c2.z;
	const float z0 = a.c0.z * b.c0.x + a.c1.z * b.c0.y + a.c2.z * b.c0.z;
	const float z1 = a.c0.z * b.c1.x + a.c1.z * b.c1.y + a.c2.z * b.c1.z;
	const float z2 = a.c0.z * b.c2.x + a.c1.z * b.c2.y + a.c2.z * b.c2.z;

	return (m3) {
		x0, y0, z0,
		x1, y1, z1,
		x2, y2, z2,
	};
}

#define M2_DIAG(X, Y)       ((m2) { X, 0.f, 0.f, Y })
#define M3_DIAG(X, Y, Z)    ((m3) { X, 0.f, 0.f, 0.f, Y, 0.f, 0.f, 0.f, Z })
#define M4_DIAG(X, Y, Z, W) ((m4)\
	{ X, 0.f, 0.f, 0.f, \
	  0.f, Y, 0.f, 0.f, \
	  0.f, 0.f, Z, 0.f, \
	  0.f, 0.f, 0.f, W, })

#define DIAG(N) static m ## N m ## N ## _diag(v ## N v) \
{ \
	m ## N m = M ## N ## _ZERO; \
	for (size_t i = 0; i < N; ++i) \
		m.s[i + N * i] = v.s[i]; \
	return m; \
}

DIAG(2);
DIAG(3);
DIAG(4);
#undef DIAG

#define FILL(A, B) static m ## A m ## A ## _fill_m ## B (m ## B n) \
{ \
	m ## A m; \
	for (size_t i = 0; i < B; ++i) { \
		for (size_t j = 0; j < B; ++j) { \
			m.v[i].s[j] = n.v[i].s[j]; \
		} \
		for (size_t j = B; j < A; ++j) { \
			m.v[i].s[j] = 0.f; \
		} \
	} \
	for (size_t i = B; i < A; ++i) { \
		m.v[i] = V ## A ## _ZERO; \
	} \
	return m; \
}

FILL(3, 2)
FILL(4, 2)
FILL(4, 3)
#undef FILL

#define TRACE(N) static float m ## N ## _trace(m ## N m) \
{ \
	float result = 0.f; \
	for (u8 i = 0; i < N; ++i) \
		result += m.s[i + N * i]; \
	return result; \
}

TRACE(2)
TRACE(3)
TRACE(4)
#undef TRACE

static v4 m3_to_qt(m3 m)
{
	const float trace = m3_trace(m);
	const float a = m.v[0].s[0];
	const float b = m.v[1].s[1];
	const float c = m.v[2].s[2];

	// TODO: debug check special orthogonal

	if (trace > 0.f) {
		const float s = 2.f * sqrtf(1.f + trace);
		return (v4) {
			(m.c1.z - m.c2.y) / s,
			(m.c2.x - m.c0.z) / s,
			(m.c0.y - m.c1.x) / s,
			.25f * s,
		};
	} else if ((a > b) && (a > c)) {
		float s = 2.f * sqrtf(1.f + a - b - c);
		return (v4) {
			.25f * s,
			(m.c1.x + m.c0.y) / s,
			(m.c2.x + m.c0.z) / s,
			(m.c1.z - m.c2.y) / s,
		};
	} else if (b > c) {
		const float s = 2.f * sqrtf(1.f + b - a - c);
		return (v4) {
			(m.c1.x + m.c0.y) / s,
			.25f * s,
			(m.c2.y + m.c1.z) / s,
			(m.c2.x - m.c0.z) / s,
		};
	} else {
		const float s = 2.f * sqrtf(1.f + c - a - b);
		return (v4) {
			(m.c2.x + m.c0.z) / s,
			(m.c2.y + m.c1.z) / s,
			.25f * s,
			(m.c0.y - m.c1.x) / s,
		};
	}
}

static ALG_INLINE m4 m4_trans(v3 v)
{
	m4 m = M4_ID;
	m.c3.xyz = v;
	return m;
}

static ALG_INLINE m4 m4_scale(float s)
{
	return M4_DIAG(s, s, s, 1.f);
}

static ALG_INLINE m4 m4_scale_aniso(float x, float y, float z)
{
	return M4_DIAG(x, y, z, 1.f);
}

// Expects VFOV (width / height) in degrees
static m4 m4_persp(float fov, float asp, float near, float far)
{
	float range = far - near;
	float z_scale = 1.f / range;
	float z_off = -near / range;

	float y_scale = 1.f / tanf(.5f * fov * M_PI / 180.f);
	float x_scale = y_scale / asp;
	y_scale *= -1.f;

	m4 m = {
		.c0.x = x_scale,
		.c1.y = y_scale,
		.c2.z = z_scale,
		.c2.w = 1.f,
		.c3.z = z_off,
	};

	return m;
}

// Scale -> number of units that can fit in vertical height
static m4 m4_ortho(float scale, float asp, float near, float far)
{
	const float range = far - near;
	const float z_scale = 1.f / range;
	const float z_off = -near / range;

	m4 m = {
		.c0.x =  2.f / (asp * scale),
		.c1.y = -2.f / scale,
		.c2.z = z_scale,
		.c3.w = 1.f,
		.c3.z = z_off,
	};

	return m;
}

/* Quaternions */

#define QT_ID ((v4) { 0.f, 0.f, 0.f, 1.f })

static inline v4 qt_conj(v4 q)
{
	return (v4) { -q.x, -q.y, -q.z, q.w, };
}

// Hamilton convention
static v4 qt_mul(v4 a, v4 b)
{
	v4 c, d;

	c = a;
	c.z *= -1.f;
	d.x = b.w;
	d.y = b.z;
	d.z = b.y;
	d.w = b.x;
	const float x = v4_dot(c, d);

	c = a;
	c.x *= -1.f;
	d = v4_shift(b, 2);
	const float y = v4_dot(c, d);

	c = a;
	c.y *= -1.f;
	d.x = b.y;
	d.y = b.x;
	d.z = b.w;
	d.w = b.z;
	const float z = v4_dot(c, d);

	c = v4_neg(a);
	c.w *= -1.f;
	d = b;
	const float w = v4_dot(c, d);

	return (v4) { x, y, z, w };
}

static v4 qt_vecs(const v3 fwd, const v3 up)
{
	m3 m = {
		.c0 = v3_cross(up, fwd),
		.c1 = up,
		.c2 = fwd,
	};

	return m3_to_qt(m);
}

// Assumes fwd and up are not orthogonal;
// will modify input fwd vector.
static v4 qt_vecs_up(v3 fwd, const v3 up)
{
#ifdef ALG_DEBUG
	assert(v3_isnorm(up));
#endif
	v3 right = v3_cross(up, fwd);
	right = v3_norm(right);
	fwd = v3_cross(right, up);

	m3 m = {
		.c0 = right,
		.c1 = up,
		.c2 = fwd,
	};

	return m3_to_qt(m);
}

// Assumes fwd and up are not orthogonal;
// will modify input up vector.
static v4 qt_vecs_fwd(const v3 fwd, v3 up)
{
#ifdef ALG_DEBUG
	assert(v3_isnorm(fwd));
#endif
	v3 right = v3_cross(up, fwd);
	right = v3_norm(right);
	up = v3_cross(fwd, right);

	m3 m = {
		.c0 = right,
		.c1 = up,
		.c2 = fwd,
	};

	return m3_to_qt(m);
}

static v4 qt_axis_angle(v3 axis, float angle)
{
#ifdef ALG_DEBUG
	assert(v3_isnorm(axis));
#endif
	float half = .5f * angle;
	float sin_half = sinf(half);
	float cos_half = cosf(half);

	return (v4) {
		axis.x * sin_half,
		axis.y * sin_half,
		axis.z * sin_half,
		cos_half,
	};
}

/* Optimized formulation:
 * gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
 */
static v3 qt_app(v4 q, v3 v)
{
	fff b = q.xyz;
	float s = q.w;

	return v3_add(
		v3_mul(b, 2.f * v3_dot(v, b)),
		v3_add(
			v3_mul(v, s * s - v3_dot(b, b)),
			v3_mul(v3_cross(b, v), 2.f * s)
		)
	);
}

static v4 qt_nlerp(v4 a, v4 b, float t)
{
	return v4_norm(
		v4_add(a, v4_mul(v4_sub(b, a), t))
	);
}

// Hamilton convention
static m3 qt_to_m3(v4 q)
{
	const float xx = q.x * q.x;
	const float yy = q.y * q.y;
	const float zz = q.z * q.z;
	const float ww = q.w * q.w;
	const float xy = q.x * q.y;
	const float zw = q.z * q.w;
	const float xz = q.x * q.z;
	const float yw = q.y * q.w;
	const float yz = q.y * q.z;
	const float xw = q.x * q.w;

	float inv = 1.f / (xx + yy + zz + ww);
	float x =  inv * ( xx - yy - zz + ww);
	float y =  inv * (-xx + yy - zz + ww);
	float z =  inv * (-xx - yy + zz + ww);

	float _2inv = 2.f * inv;

	return (m3) {
		.c0 = { x, _2inv * (xy + zw), _2inv * (xz - yw) },
		.c1 = { _2inv * (xy - zw), y, _2inv * (yz + xw) },
		.c2 = { _2inv * (xz + yw), _2inv * (yz - xw), z },
	};
}

static m4 qt_to_m4(v4 q)
{
	m4 m = m4_fill_m3(qt_to_m3(q));
	m.s[15] = 1.f;
	return m;
}

/* Runoff */

static m4 m4_model(v3 pos, v4 rot, float scale)
{
	const m3 s = M3_DIAG(scale, scale, scale);
	const m3 r = qt_to_m3(rot);
	const m3 rs = m3_mul(r, s);

	m4 result = {
		.c0 = { rs.c0.x, rs.c0.y, rs.c0.z, 0.f },
		.c1 = { rs.c1.x, rs.c1.y, rs.c1.z, 0.f },
		.c2 = { rs.c2.x, rs.c2.y, rs.c2.z, 0.f },
		.c3 = {   pos.x,   pos.y,   pos.z, 1.f },
	};

	return result;
}

static m4 m4_view(v3 pos, v4 rot)
{
	return m4_mul(qt_to_m4(qt_conj(rot)), m4_trans(v3_neg(pos)));
}

/* Debug */

#include <stdio.h>

static inline void fprintf32(FILE *stream, float s)
{
	fprintf(stream, "%8.4f\n", s);
}

static inline void printf32(float s)
{
	fprintf(stdout, "%8.4f\n", s);
}

#define fprint(N, ID) static void ID(FILE *stream, v ## N v) \
{ \
	for (size_t i = 0; i < N; ++i) \
		fprintf(stream, "%8.4f  ", v.s[i]); \
	fprintf(stream, "\n"); \
}

GEN(2, fprint, ff)
GEN(3, fprint, fff)
GEN(4, fprint, ffff)
#undef fprint

#define print(N, ID) static void ID(v ## N v) \
{ \
	v ## N ## _fprint(stdout, v); \
}

GEN(2, print, ff)
GEN(3, print, fff)
GEN(4, print, ffff)
#undef print

#define FPRINT(N) static void m ## N ## _fprint(FILE *stream, m ## N m) \
{ \
	for (size_t i = 0; i < N; ++i) { \
		for (size_t j = 0; j < N; ++j) { \
			fprintf(stream, "%8.4f  ", m.v[j].s[i]); \
		} \
		fprintf(stream, "\n"); \
	} \
}

FPRINT(2)
FPRINT(3)
FPRINT(4)
#undef FPRINT

#define PRINT(N) static void m ## N ## _print(m ## N m) \
{ \
	m ## N ## _fprint(stdout, m); \
}

PRINT(2)
PRINT(3)
PRINT(4)
#undef PRINT

/* Vector generators (1) */

#undef  GEN
#define GEN(N, ID) ID(N, v ## N ## _ ## ID)

#define genx(N, ID) static v ## N ID(float s) { return (v ## N) { .x = s }; }
 GEN(2, genx)
 GEN(3, genx)
 GEN(4, genx)
#undef  genx

#define geny(N, ID) static v ## N ID(float s) { return (v ## N) { .y = s }; }
 GEN(2, geny)
 GEN(3, geny)
 GEN(4, geny)
#undef  geny

#define genz(N, ID) static v ## N ID(float s) { return (v ## N) { .z = s }; }
 GEN(3, genz)
 GEN(4, genz)
#undef  genz

#define genw(N, ID) static v ## N ID(float s) { return (v ## N) { .w = s }; }
 GEN(4, genw)
#undef  genw

#define padx(N, ID) static v ## N ID(float s, float p) \
{ \
	v ## N result = V ## N ## _FILL(p); \
	result.x = s; \
	return result; \
}

 GEN(2, padx)
 GEN(3, padx)
 GEN(4, padx)
#undef  padx

#define pady(N, ID) static v ## N ID(float s, float p) \
{ \
	v ## N result = V ## N ## _FILL(p); \
	result.y = s; \
	return result; \
}

 GEN(2, pady)
 GEN(3, pady)
 GEN(4, pady)
#undef  pady

#define padz(N, ID) static v ## N ID(float s, float p) \
{ \
	v ## N result = V ## N ## _FILL(p); \
	result.z = s; \
	return result; \
}

 GEN(3, padz)
 GEN(4, padz)
#undef  padz

#define padw(N, ID) static v ## N ID(float s, float p) \
{ \
	v ## N result = V ## N ## _FILL(p); \
	result.w = s; \
	return result; \
}

 GEN(4, padw)
#undef  padw

/* Vector generators (2) */

#define MKF2(N, A, B, PRE, PST) \
static v ## N PRE ## gen ## PST (float a, float b) \
{ \
	return (v ## N) { . A = a, . B = b }; \
} \
static v ## N PRE ## pad ## PST (v2 v, float p) \
{ \
	v ## N result = V ## N ## _FILL(p); \
	result . A = v.x; \
	result . B = v.y; \
	return result; \
}

#define GEN2(N, A, B) \
	MKF2(N, A, B, v ## N ## _, A ## B)

GEN2(2, x, y) // (!)
GEN2(3, x, y)
GEN2(4, x, y)
GEN2(3, x, z)
GEN2(4, x, z)
GEN2(4, x, w)
GEN2(3, y, z)
GEN2(4, y, z)
GEN2(4, y, w)
GEN2(4, z, w)

#undef MKF2
#undef GEN2

/* Vector generators (3) */

#define MKF3(N, A, B, C, PRE, PST) \
static v ## N PRE ## gen ## PST (float a, float b, float c) \
{ \
	return (v ## N) { .A = a, .B = b, .C = c }; \
} \
static v ## N PRE ## pad ## PST (v3 v, float p) \
{ \
	v ## N result = V ## N ## _FILL(p); \
	result . A = v.x; \
	result . B = v.y; \
	result . C = v.z; \
	return result; \
}

#define GEN3(N, A, B, C) \
	MKF3(N, A, B, C, v ## N ## _, A ## B ## C)

GEN3(3, x, y, z) // (!)
GEN3(4, x, y, z)
GEN3(4, x, y, w)
GEN3(4, x, z, w)
GEN3(4, y, z, w)

#undef MKF3
#undef GEN3

/* Vector generators (4) */

#define genxyzw(N, ID) static v ## N ID(float a, float b, float c, float d) \
{ \
	return (v ## N) { .x = a, .y = b, .z = c, .w = d }; \
}

GEN(4, genxyzw)
#undef genxyzw

/* Swizzles (2) */

#define XX(N) static v2 v ## N ## _xx(v ## N v) { return (v2) { v.x, v.x }; }
        XX(2)
        XX(3)
        XX(4)
#undef  XX
#define XY(N) static v2 v ## N ## _xy(v ## N v) { return (v2) { v.x, v.y }; }
        XY(2) // (!)
        XY(3)
        XY(4)
#undef  XY
#define XZ(N) static v2 v ## N ## _xz(v ## N v) { return (v2) { v.x, v.z }; }
        XZ(3)
        XZ(4)
#undef  XZ
#define XW(N) static v2 v ## N ## _xw(v ## N v) { return (v2) { v.x, v.w }; }
        XW(4)
#undef  XW

#define YX(N) static v2 v ## N ## _yx(v ## N v) { return (v2) { v.y, v.x }; }
        YX(2)
        YX(3)
        YX(4)
#undef  YX
#define YY(N) static v2 v ## N ## _yy(v ## N v) { return (v2) { v.y, v.y }; }
        YY(2)
        YY(3)
        YY(4)
#undef  YY
#define YZ(N) static v2 v ## N ## _yz(v ## N v) { return (v2) { v.y, v.z }; }
        YZ(3)
        YZ(4)
#undef  YZ
#define YW(N) static v2 v ## N ## _yw(v ## N v) { return (v2) { v.y, v.w }; }
        YW(4)
#undef  YW

#define ZX(N) static v2 v ## N ## _zx(v ## N v) { return (v2) { v.z, v.x }; }
        ZX(3)
        ZX(4)
#undef  ZX
#define ZY(N) static v2 v ## N ## _zy(v ## N v) { return (v2) { v.z, v.y }; }
        ZY(3)
        ZY(4)
#undef  ZY
#define ZZ(N) static v2 v ## N ## _zz(v ## N v) { return (v2) { v.z, v.z }; }
        ZZ(3)
        ZZ(4)
#undef  ZZ
#define ZW(N) static v2 v ## N ## _zw(v ## N v) { return (v2) { v.z, v.w }; }
        ZW(4)
#undef  ZW

#define WX(N) static v2 v ## N ## _wx(v ## N v) { return (v2) { v.w, v.x }; }
        WX(4)
#undef  WX
#define WY(N) static v2 v ## N ## _wy(v ## N v) { return (v2) { v.w, v.y }; }
        WY(4)
#undef  WY
#define WZ(N) static v2 v ## N ## _wz(v ## N v) { return (v2) { v.w, v.z }; }
        WZ(4)
#undef  WZ
#define WW(N) static v2 v ## N ## _ww(v ## N v) { return (v2) { v.w, v.w }; }
        WW(4)
#undef  WW

/* Swizzles (3) */

#define XXX(N) static v3 v ## N ## _xxx(v ## N v) { return (v3) { v.x, v.x, v.x }; }
        XXX(3)
        XXX(4)
#undef  XXX
#define XXY(N) static v3 v ## N ## _xxy(v ## N v) { return (v3) { v.x, v.x, v.y }; }
        XXY(3)
        XXY(4)
#undef  XXY
#define XXZ(N) static v3 v ## N ## _xxz(v ## N v) { return (v3) { v.x, v.x, v.z }; }
        XXZ(3)
        XXZ(4)
#undef  XXZ
#define XXW(N) static v3 v ## N ## _xxw(v ## N v) { return (v3) { v.x, v.x, v.w }; }
        XXW(4)
#undef  XXW
#define XYX(N) static v3 v ## N ## _xyx(v ## N v) { return (v3) { v.x, v.y, v.x }; }
        XYX(3)
        XYX(4)
#undef  XYX
#define XYY(N) static v3 v ## N ## _xyy(v ## N v) { return (v3) { v.x, v.y, v.y }; }
        XYY(3)
        XYY(4)
#undef  XYY
#define XYZ(N) static v3 v ## N ## _xyz(v ## N v) { return (v3) { v.x, v.y, v.z }; }
        XYZ(3) // (!)
        XYZ(4)
#undef  XYZ
#define XYW(N) static v3 v ## N ## _xyw(v ## N v) { return (v3) { v.x, v.y, v.w }; }
        XYW(4)
#undef  XYW
#define XZX(N) static v3 v ## N ## _xzx(v ## N v) { return (v3) { v.x, v.z, v.x }; }
        XZX(3)
        XZX(4)
#undef  XZX
#define XZY(N) static v3 v ## N ## _xzy(v ## N v) { return (v3) { v.x, v.z, v.y }; }
        XZY(3)
        XZY(4)
#undef  XZY
#define XZZ(N) static v3 v ## N ## _xzz(v ## N v) { return (v3) { v.x, v.z, v.z }; }
        XZZ(3)
        XZZ(4)
#undef  XZZ
#define XZW(N) static v3 v ## N ## _xzw(v ## N v) { return (v3) { v.x, v.z, v.w }; }
        XZW(4)
#undef  XZW
#define XWX(N) static v3 v ## N ## _xwx(v ## N v) { return (v3) { v.x, v.w, v.x }; }
        XWX(4)
#undef  XWX
#define XWY(N) static v3 v ## N ## _xwy(v ## N v) { return (v3) { v.x, v.w, v.y }; }
        XWY(4)
#undef  XWY
#define XWZ(N) static v3 v ## N ## _xwz(v ## N v) { return (v3) { v.x, v.w, v.z }; }
        XWZ(4)
#undef  XWZ
#define XWW(N) static v3 v ## N ## _xww(v ## N v) { return (v3) { v.x, v.w, v.w }; }
        XWW(4)
#undef  XWW

#define YXX(N) static v3 v ## N ## _yxx(v ## N v) { return (v3) { v.y, v.x, v.x }; }
        YXX(3)
        YXX(4)
#undef  YXX
#define YXY(N) static v3 v ## N ## _yxy(v ## N v) { return (v3) { v.y, v.x, v.y }; }
        YXY(3)
        YXY(4)
#undef  YXY
#define YXZ(N) static v3 v ## N ## _yxz(v ## N v) { return (v3) { v.y, v.x, v.z }; }
        YXZ(3)
        YXZ(4)
#undef  YXZ
#define YXW(N) static v3 v ## N ## _yxw(v ## N v) { return (v3) { v.y, v.x, v.w }; }
        YXW(4)
#define YYX(N) static v3 v ## N ## _yyx(v ## N v) { return (v3) { v.y, v.y, v.x }; }
        YYX(3)
        YYX(4)
#undef  YYX
#define YYY(N) static v3 v ## N ## _yyy(v ## N v) { return (v3) { v.y, v.y, v.y }; }
        YYY(3)
        YYY(4)
#undef  YYY
#define YYZ(N) static v3 v ## N ## _yyz(v ## N v) { return (v3) { v.y, v.y, v.z }; }
        YYZ(3)
        YYZ(4)
#undef  YYZ
#define YYW(N) static v3 v ## N ## _yyw(v ## N v) { return (v3) { v.y, v.y, v.w }; }
        YYW(4)
#undef  YYW
#define YZX(N) static v3 v ## N ## _yzx(v ## N v) { return (v3) { v.y, v.z, v.x }; }
        YZX(3)
        YZX(4)
#undef  YZX
#define YZY(N) static v3 v ## N ## _yzy(v ## N v) { return (v3) { v.y, v.z, v.y }; }
        YZY(3)
        YZY(4)
#undef  YZY
#define YZZ(N) static v3 v ## N ## _yzz(v ## N v) { return (v3) { v.y, v.z, v.z }; }
        YZZ(3)
        YZZ(4)
#undef  YZZ
#define YZW(N) static v3 v ## N ## _yzw(v ## N v) { return (v3) { v.y, v.z, v.w }; }
        YZW(4)
#undef  YZW
#define YWX(N) static v3 v ## N ## _ywx(v ## N v) { return (v3) { v.y, v.w, v.x }; }
        YWX(4)
#undef  YWX
#define YWY(N) static v3 v ## N ## _ywy(v ## N v) { return (v3) { v.y, v.w, v.y }; }
        YWY(4)
#undef  YWY
#define YWZ(N) static v3 v ## N ## _ywz(v ## N v) { return (v3) { v.y, v.w, v.z }; }
        YWZ(4)
#undef  YWZ
#define YWW(N) static v3 v ## N ## _yww(v ## N v) { return (v3) { v.y, v.w, v.w }; }
        YWW(4)
#undef  YWW

#define ZXX(N) static v3 v ## N ## _zxx(v ## N v) { return (v3) { v.z, v.x, v.x }; }
        ZXX(3)
        ZXX(4)
#undef  ZXX
#define ZXY(N) static v3 v ## N ## _zxy(v ## N v) { return (v3) { v.z, v.x, v.y }; }
        ZXY(3)
        ZXY(4)
#undef  ZXY
#define ZXZ(N) static v3 v ## N ## _zxz(v ## N v) { return (v3) { v.z, v.x, v.z }; }
        ZXZ(3)
        ZXZ(4)
#undef  ZXZ
#define ZXW(N) static v3 v ## N ## _zxw(v ## N v) { return (v3) { v.z, v.x, v.w }; }
        ZXW(4)
#define ZYX(N) static v3 v ## N ## _zyx(v ## N v) { return (v3) { v.z, v.y, v.x }; }
        ZYX(3)
        ZYX(4)
#undef  ZYX
#define ZYY(N) static v3 v ## N ## _zyy(v ## N v) { return (v3) { v.z, v.y, v.y }; }
        ZYY(3)
        ZYY(4)
#undef  ZYY
#define ZYZ(N) static v3 v ## N ## _zyz(v ## N v) { return (v3) { v.z, v.y, v.z }; }
        ZYZ(3)
        ZYZ(4)
#undef  ZYZ
#define ZYW(N) static v3 v ## N ## _zyw(v ## N v) { return (v3) { v.z, v.y, v.w }; }
        ZYW(4)
#undef  ZYW
#define ZZX(N) static v3 v ## N ## _zzx(v ## N v) { return (v3) { v.z, v.z, v.x }; }
        ZZX(3)
        ZZX(4)
#undef  ZZX
#define ZZY(N) static v3 v ## N ## _zzy(v ## N v) { return (v3) { v.z, v.z, v.y }; }
        ZZY(3)
        ZZY(4)
#undef  ZZY
#define ZZZ(N) static v3 v ## N ## _zzz(v ## N v) { return (v3) { v.z, v.z, v.z }; }
        ZZZ(3)
        ZZZ(4)
#undef  ZZZ
#define ZZW(N) static v3 v ## N ## _zzw(v ## N v) { return (v3) { v.z, v.z, v.w }; }
        ZZW(4)
#undef  ZZW
#define ZWX(N) static v3 v ## N ## _zwx(v ## N v) { return (v3) { v.z, v.w, v.x }; }
        ZWX(4)
#undef  ZWX
#define ZWY(N) static v3 v ## N ## _zwy(v ## N v) { return (v3) { v.z, v.w, v.y }; }
        ZWY(4)
#undef  ZWY
#define ZWZ(N) static v3 v ## N ## _zwz(v ## N v) { return (v3) { v.z, v.w, v.z }; }
        ZWZ(4)
#undef  ZWZ
#define ZWW(N) static v3 v ## N ## _zww(v ## N v) { return (v3) { v.z, v.w, v.w }; }
        ZWW(4)
#undef  ZWW

#define WXX(N) static v3 v ## N ## _wxx(v ## N v) { return (v3) { v.w, v.x, v.x }; }
        WXX(4)
#undef  WXX
#define WXY(N) static v3 v ## N ## _wxy(v ## N v) { return (v3) { v.w, v.x, v.y }; }
        WXY(4)
#undef  WXY
#define WXZ(N) static v3 v ## N ## _wxz(v ## N v) { return (v3) { v.w, v.x, v.z }; }
        WXZ(4)
#undef  WXZ
#define WXW(N) static v3 v ## N ## _wxw(v ## N v) { return (v3) { v.w, v.x, v.w }; }
        WXW(4)
#define WYX(N) static v3 v ## N ## _wyx(v ## N v) { return (v3) { v.w, v.y, v.x }; }
        WYX(4)
#undef  WYX
#define WYY(N) static v3 v ## N ## _wyy(v ## N v) { return (v3) { v.w, v.y, v.y }; }
        WYY(4)
#undef  WYY
#define WYZ(N) static v3 v ## N ## _wyz(v ## N v) { return (v3) { v.w, v.y, v.z }; }
        WYZ(4)
#undef  WYZ
#define WYW(N) static v3 v ## N ## _wyw(v ## N v) { return (v3) { v.w, v.y, v.w }; }
        WYW(4)
#undef  WYW
#define WZX(N) static v3 v ## N ## _wzx(v ## N v) { return (v3) { v.w, v.z, v.x }; }
        WZX(4)
#undef  WZX
#define WZY(N) static v3 v ## N ## _wzy(v ## N v) { return (v3) { v.w, v.z, v.y }; }
        WZY(4)
#undef  WZY
#define WZZ(N) static v3 v ## N ## _wzz(v ## N v) { return (v3) { v.w, v.z, v.z }; }
        WZZ(4)
#undef  WZZ
#define WZW(N) static v3 v ## N ## _wzw(v ## N v) { return (v3) { v.w, v.z, v.w }; }
        WZW(4)
#undef  WZW
#define WWX(N) static v3 v ## N ## _wwx(v ## N v) { return (v3) { v.w, v.w, v.x }; }
        WWX(4)
#undef  WWX
#define WWY(N) static v3 v ## N ## _wwy(v ## N v) { return (v3) { v.w, v.w, v.y }; }
        WWY(4)
#undef  WWY
#define WWZ(N) static v3 v ## N ## _wwz(v ## N v) { return (v3) { v.w, v.w, v.z }; }
        WWZ(4)
#undef  WWZ
#define WWW(N) static v3 v ## N ## _www(v ## N v) { return (v3) { v.w, v.w, v.w }; }
        WWW(4)
#undef  WWW

/* Swizzles (4) */

static v4 v4_xxxx(v4 v) { return (v4) { v.x, v.x, v.x, v.x }; }
static v4 v4_xxxy(v4 v) { return (v4) { v.x, v.x, v.x, v.y }; }
static v4 v4_xxxz(v4 v) { return (v4) { v.x, v.x, v.x, v.z }; }
static v4 v4_xxxw(v4 v) { return (v4) { v.x, v.x, v.x, v.w }; }
static v4 v4_xxyx(v4 v) { return (v4) { v.x, v.x, v.y, v.x }; }
static v4 v4_xxyy(v4 v) { return (v4) { v.x, v.x, v.y, v.y }; }
static v4 v4_xxyz(v4 v) { return (v4) { v.x, v.x, v.y, v.z }; }
static v4 v4_xxyw(v4 v) { return (v4) { v.x, v.x, v.y, v.w }; }
static v4 v4_xxzx(v4 v) { return (v4) { v.x, v.x, v.z, v.x }; }
static v4 v4_xxzy(v4 v) { return (v4) { v.x, v.x, v.z, v.y }; }
static v4 v4_xxzz(v4 v) { return (v4) { v.x, v.x, v.z, v.z }; }
static v4 v4_xxzw(v4 v) { return (v4) { v.x, v.x, v.z, v.w }; }
static v4 v4_xxwx(v4 v) { return (v4) { v.x, v.x, v.w, v.x }; }
static v4 v4_xxwy(v4 v) { return (v4) { v.x, v.x, v.w, v.y }; }
static v4 v4_xxwz(v4 v) { return (v4) { v.x, v.x, v.w, v.z }; }
static v4 v4_xxww(v4 v) { return (v4) { v.x, v.x, v.w, v.w }; }
static v4 v4_xyxx(v4 v) { return (v4) { v.x, v.y, v.x, v.x }; }
static v4 v4_xyxy(v4 v) { return (v4) { v.x, v.y, v.x, v.y }; }
static v4 v4_xyxz(v4 v) { return (v4) { v.x, v.y, v.x, v.z }; }
static v4 v4_xyxw(v4 v) { return (v4) { v.x, v.y, v.x, v.w }; }
static v4 v4_xyyx(v4 v) { return (v4) { v.x, v.y, v.y, v.x }; }
static v4 v4_xyyy(v4 v) { return (v4) { v.x, v.y, v.y, v.y }; }
static v4 v4_xyyz(v4 v) { return (v4) { v.x, v.y, v.y, v.z }; }
static v4 v4_xyyw(v4 v) { return (v4) { v.x, v.y, v.y, v.w }; }
static v4 v4_xyzx(v4 v) { return (v4) { v.x, v.y, v.z, v.x }; }
static v4 v4_xyzy(v4 v) { return (v4) { v.x, v.y, v.z, v.y }; }
static v4 v4_xyzz(v4 v) { return (v4) { v.x, v.y, v.z, v.z }; }
static v4 v4_xyzw(v4 v) { return (v4) { v.x, v.y, v.z, v.w }; } // (!)
static v4 v4_xywx(v4 v) { return (v4) { v.x, v.y, v.w, v.x }; }
static v4 v4_xywy(v4 v) { return (v4) { v.x, v.y, v.w, v.y }; }
static v4 v4_xywz(v4 v) { return (v4) { v.x, v.y, v.w, v.z }; }
static v4 v4_xyww(v4 v) { return (v4) { v.x, v.y, v.w, v.w }; }
static v4 v4_xzxx(v4 v) { return (v4) { v.x, v.z, v.x, v.x }; }
static v4 v4_xzxy(v4 v) { return (v4) { v.x, v.z, v.x, v.y }; }
static v4 v4_xzxz(v4 v) { return (v4) { v.x, v.z, v.x, v.z }; }
static v4 v4_xzxw(v4 v) { return (v4) { v.x, v.z, v.x, v.w }; }
static v4 v4_xzyx(v4 v) { return (v4) { v.x, v.z, v.y, v.x }; }
static v4 v4_xzyy(v4 v) { return (v4) { v.x, v.z, v.y, v.y }; }
static v4 v4_xzyz(v4 v) { return (v4) { v.x, v.z, v.y, v.z }; }
static v4 v4_xzyw(v4 v) { return (v4) { v.x, v.z, v.y, v.w }; }
static v4 v4_xzzx(v4 v) { return (v4) { v.x, v.z, v.z, v.x }; }
static v4 v4_xzzy(v4 v) { return (v4) { v.x, v.z, v.z, v.y }; }
static v4 v4_xzzz(v4 v) { return (v4) { v.x, v.z, v.z, v.z }; }
static v4 v4_xzzw(v4 v) { return (v4) { v.x, v.z, v.z, v.w }; }
static v4 v4_xzwx(v4 v) { return (v4) { v.x, v.z, v.w, v.x }; }
static v4 v4_xzwy(v4 v) { return (v4) { v.x, v.z, v.w, v.y }; }
static v4 v4_xzwz(v4 v) { return (v4) { v.x, v.z, v.w, v.z }; }
static v4 v4_xzww(v4 v) { return (v4) { v.x, v.z, v.w, v.w }; }
static v4 v4_xwxx(v4 v) { return (v4) { v.x, v.w, v.x, v.x }; }
static v4 v4_xwxy(v4 v) { return (v4) { v.x, v.w, v.x, v.y }; }
static v4 v4_xwxz(v4 v) { return (v4) { v.x, v.w, v.x, v.z }; }
static v4 v4_xwxw(v4 v) { return (v4) { v.x, v.w, v.x, v.w }; }
static v4 v4_xwyx(v4 v) { return (v4) { v.x, v.w, v.y, v.x }; }
static v4 v4_xwyy(v4 v) { return (v4) { v.x, v.w, v.y, v.y }; }
static v4 v4_xwyz(v4 v) { return (v4) { v.x, v.w, v.y, v.z }; }
static v4 v4_xwyw(v4 v) { return (v4) { v.x, v.w, v.y, v.w }; }
static v4 v4_xwzx(v4 v) { return (v4) { v.x, v.w, v.z, v.x }; }
static v4 v4_xwzy(v4 v) { return (v4) { v.x, v.w, v.z, v.y }; }
static v4 v4_xwzz(v4 v) { return (v4) { v.x, v.w, v.z, v.z }; }
static v4 v4_xwzw(v4 v) { return (v4) { v.x, v.w, v.z, v.w }; }
static v4 v4_xwwx(v4 v) { return (v4) { v.x, v.w, v.w, v.x }; }
static v4 v4_xwwy(v4 v) { return (v4) { v.x, v.w, v.w, v.y }; }
static v4 v4_xwwz(v4 v) { return (v4) { v.x, v.w, v.w, v.z }; }
static v4 v4_xwww(v4 v) { return (v4) { v.x, v.w, v.w, v.w }; }

static v4 v4_yxxx(v4 v) { return (v4) { v.y, v.x, v.x, v.x }; }
static v4 v4_yxxy(v4 v) { return (v4) { v.y, v.x, v.x, v.y }; }
static v4 v4_yxxz(v4 v) { return (v4) { v.y, v.x, v.x, v.z }; }
static v4 v4_yxxw(v4 v) { return (v4) { v.y, v.x, v.x, v.w }; }
static v4 v4_yxyx(v4 v) { return (v4) { v.y, v.x, v.y, v.x }; }
static v4 v4_yxyy(v4 v) { return (v4) { v.y, v.x, v.y, v.y }; }
static v4 v4_yxyz(v4 v) { return (v4) { v.y, v.x, v.y, v.z }; }
static v4 v4_yxyw(v4 v) { return (v4) { v.y, v.x, v.y, v.w }; }
static v4 v4_yxzx(v4 v) { return (v4) { v.y, v.x, v.z, v.x }; }
static v4 v4_yxzy(v4 v) { return (v4) { v.y, v.x, v.z, v.y }; }
static v4 v4_yxzz(v4 v) { return (v4) { v.y, v.x, v.z, v.z }; }
static v4 v4_yxzw(v4 v) { return (v4) { v.y, v.x, v.z, v.w }; }
static v4 v4_yxwx(v4 v) { return (v4) { v.y, v.x, v.w, v.x }; }
static v4 v4_yxwy(v4 v) { return (v4) { v.y, v.x, v.w, v.y }; }
static v4 v4_yxwz(v4 v) { return (v4) { v.y, v.x, v.w, v.z }; }
static v4 v4_yxww(v4 v) { return (v4) { v.y, v.x, v.w, v.w }; }
static v4 v4_yyxx(v4 v) { return (v4) { v.y, v.y, v.x, v.x }; }
static v4 v4_yyxy(v4 v) { return (v4) { v.y, v.y, v.x, v.y }; }
static v4 v4_yyxz(v4 v) { return (v4) { v.y, v.y, v.x, v.z }; }
static v4 v4_yyxw(v4 v) { return (v4) { v.y, v.y, v.x, v.w }; }
static v4 v4_yyyx(v4 v) { return (v4) { v.y, v.y, v.y, v.x }; }
static v4 v4_yyyy(v4 v) { return (v4) { v.y, v.y, v.y, v.y }; }
static v4 v4_yyyz(v4 v) { return (v4) { v.y, v.y, v.y, v.z }; }
static v4 v4_yyyw(v4 v) { return (v4) { v.y, v.y, v.y, v.w }; }
static v4 v4_yyzx(v4 v) { return (v4) { v.y, v.y, v.z, v.x }; }
static v4 v4_yyzy(v4 v) { return (v4) { v.y, v.y, v.z, v.y }; }
static v4 v4_yyzz(v4 v) { return (v4) { v.y, v.y, v.z, v.z }; }
static v4 v4_yyzw(v4 v) { return (v4) { v.y, v.y, v.z, v.w }; }
static v4 v4_yywx(v4 v) { return (v4) { v.y, v.y, v.w, v.x }; }
static v4 v4_yywy(v4 v) { return (v4) { v.y, v.y, v.w, v.y }; }
static v4 v4_yywz(v4 v) { return (v4) { v.y, v.y, v.w, v.z }; }
static v4 v4_yyww(v4 v) { return (v4) { v.y, v.y, v.w, v.w }; }
static v4 v4_yzxx(v4 v) { return (v4) { v.y, v.z, v.x, v.x }; }
static v4 v4_yzxy(v4 v) { return (v4) { v.y, v.z, v.x, v.y }; }
static v4 v4_yzxz(v4 v) { return (v4) { v.y, v.z, v.x, v.z }; }
static v4 v4_yzxw(v4 v) { return (v4) { v.y, v.z, v.x, v.w }; }
static v4 v4_yzyx(v4 v) { return (v4) { v.y, v.z, v.y, v.x }; }
static v4 v4_yzyy(v4 v) { return (v4) { v.y, v.z, v.y, v.y }; }
static v4 v4_yzyz(v4 v) { return (v4) { v.y, v.z, v.y, v.z }; }
static v4 v4_yzyw(v4 v) { return (v4) { v.y, v.z, v.y, v.w }; }
static v4 v4_yzzx(v4 v) { return (v4) { v.y, v.z, v.z, v.x }; }
static v4 v4_yzzy(v4 v) { return (v4) { v.y, v.z, v.z, v.y }; }
static v4 v4_yzzz(v4 v) { return (v4) { v.y, v.z, v.z, v.z }; }
static v4 v4_yzzw(v4 v) { return (v4) { v.y, v.z, v.z, v.w }; }
static v4 v4_yzwx(v4 v) { return (v4) { v.y, v.z, v.w, v.x }; }
static v4 v4_yzwy(v4 v) { return (v4) { v.y, v.z, v.w, v.y }; }
static v4 v4_yzwz(v4 v) { return (v4) { v.y, v.z, v.w, v.z }; }
static v4 v4_yzww(v4 v) { return (v4) { v.y, v.z, v.w, v.w }; }
static v4 v4_ywxx(v4 v) { return (v4) { v.y, v.w, v.x, v.x }; }
static v4 v4_ywxy(v4 v) { return (v4) { v.y, v.w, v.x, v.y }; }
static v4 v4_ywxz(v4 v) { return (v4) { v.y, v.w, v.x, v.z }; }
static v4 v4_ywxw(v4 v) { return (v4) { v.y, v.w, v.x, v.w }; }
static v4 v4_ywyx(v4 v) { return (v4) { v.y, v.w, v.y, v.x }; }
static v4 v4_ywyy(v4 v) { return (v4) { v.y, v.w, v.y, v.y }; }
static v4 v4_ywyz(v4 v) { return (v4) { v.y, v.w, v.y, v.z }; }
static v4 v4_ywyw(v4 v) { return (v4) { v.y, v.w, v.y, v.w }; }
static v4 v4_ywzx(v4 v) { return (v4) { v.y, v.w, v.z, v.x }; }
static v4 v4_ywzy(v4 v) { return (v4) { v.y, v.w, v.z, v.y }; }
static v4 v4_ywzz(v4 v) { return (v4) { v.y, v.w, v.z, v.z }; }
static v4 v4_ywzw(v4 v) { return (v4) { v.y, v.w, v.z, v.w }; }
static v4 v4_ywwx(v4 v) { return (v4) { v.y, v.w, v.w, v.x }; }
static v4 v4_ywwy(v4 v) { return (v4) { v.y, v.w, v.w, v.y }; }
static v4 v4_ywwz(v4 v) { return (v4) { v.y, v.w, v.w, v.z }; }
static v4 v4_ywww(v4 v) { return (v4) { v.y, v.w, v.w, v.w }; }

static v4 v4_zxxx(v4 v) { return (v4) { v.z, v.x, v.x, v.x }; }
static v4 v4_zxxy(v4 v) { return (v4) { v.z, v.x, v.x, v.y }; }
static v4 v4_zxxz(v4 v) { return (v4) { v.z, v.x, v.x, v.z }; }
static v4 v4_zxxw(v4 v) { return (v4) { v.z, v.x, v.x, v.w }; }
static v4 v4_zxyx(v4 v) { return (v4) { v.z, v.x, v.y, v.x }; }
static v4 v4_zxyy(v4 v) { return (v4) { v.z, v.x, v.y, v.y }; }
static v4 v4_zxyz(v4 v) { return (v4) { v.z, v.x, v.y, v.z }; }
static v4 v4_zxyw(v4 v) { return (v4) { v.z, v.x, v.y, v.w }; }
static v4 v4_zxzx(v4 v) { return (v4) { v.z, v.x, v.z, v.x }; }
static v4 v4_zxzy(v4 v) { return (v4) { v.z, v.x, v.z, v.y }; }
static v4 v4_zxzz(v4 v) { return (v4) { v.z, v.x, v.z, v.z }; }
static v4 v4_zxzw(v4 v) { return (v4) { v.z, v.x, v.z, v.w }; }
static v4 v4_zxwx(v4 v) { return (v4) { v.z, v.x, v.w, v.x }; }
static v4 v4_zxwy(v4 v) { return (v4) { v.z, v.x, v.w, v.y }; }
static v4 v4_zxwz(v4 v) { return (v4) { v.z, v.x, v.w, v.z }; }
static v4 v4_zxww(v4 v) { return (v4) { v.z, v.x, v.w, v.w }; }
static v4 v4_zyxx(v4 v) { return (v4) { v.z, v.y, v.x, v.x }; }
static v4 v4_zyxy(v4 v) { return (v4) { v.z, v.y, v.x, v.y }; }
static v4 v4_zyxz(v4 v) { return (v4) { v.z, v.y, v.x, v.z }; }
static v4 v4_zyxw(v4 v) { return (v4) { v.z, v.y, v.x, v.w }; }
static v4 v4_zyyx(v4 v) { return (v4) { v.z, v.y, v.y, v.x }; }
static v4 v4_zyyy(v4 v) { return (v4) { v.z, v.y, v.y, v.y }; }
static v4 v4_zyyz(v4 v) { return (v4) { v.z, v.y, v.y, v.z }; }
static v4 v4_zyyw(v4 v) { return (v4) { v.z, v.y, v.y, v.w }; }
static v4 v4_zyzx(v4 v) { return (v4) { v.z, v.y, v.z, v.x }; }
static v4 v4_zyzy(v4 v) { return (v4) { v.z, v.y, v.z, v.y }; }
static v4 v4_zyzz(v4 v) { return (v4) { v.z, v.y, v.z, v.z }; }
static v4 v4_zyzw(v4 v) { return (v4) { v.z, v.y, v.z, v.w }; }
static v4 v4_zywx(v4 v) { return (v4) { v.z, v.y, v.w, v.x }; }
static v4 v4_zywy(v4 v) { return (v4) { v.z, v.y, v.w, v.y }; }
static v4 v4_zywz(v4 v) { return (v4) { v.z, v.y, v.w, v.z }; }
static v4 v4_zyww(v4 v) { return (v4) { v.z, v.y, v.w, v.w }; }
static v4 v4_zzxx(v4 v) { return (v4) { v.z, v.z, v.x, v.x }; }
static v4 v4_zzxy(v4 v) { return (v4) { v.z, v.z, v.x, v.y }; }
static v4 v4_zzxz(v4 v) { return (v4) { v.z, v.z, v.x, v.z }; }
static v4 v4_zzxw(v4 v) { return (v4) { v.z, v.z, v.x, v.w }; }
static v4 v4_zzyx(v4 v) { return (v4) { v.z, v.z, v.y, v.x }; }
static v4 v4_zzyy(v4 v) { return (v4) { v.z, v.z, v.y, v.y }; }
static v4 v4_zzyz(v4 v) { return (v4) { v.z, v.z, v.y, v.z }; }
static v4 v4_zzyw(v4 v) { return (v4) { v.z, v.z, v.y, v.w }; }
static v4 v4_zzzx(v4 v) { return (v4) { v.z, v.z, v.z, v.x }; }
static v4 v4_zzzy(v4 v) { return (v4) { v.z, v.z, v.z, v.y }; }
static v4 v4_zzzz(v4 v) { return (v4) { v.z, v.z, v.z, v.z }; }
static v4 v4_zzzw(v4 v) { return (v4) { v.z, v.z, v.z, v.w }; }
static v4 v4_zzwx(v4 v) { return (v4) { v.z, v.z, v.w, v.x }; }
static v4 v4_zzwy(v4 v) { return (v4) { v.z, v.z, v.w, v.y }; }
static v4 v4_zzwz(v4 v) { return (v4) { v.z, v.z, v.w, v.z }; }
static v4 v4_zzww(v4 v) { return (v4) { v.z, v.z, v.w, v.w }; }
static v4 v4_zwxx(v4 v) { return (v4) { v.z, v.w, v.x, v.x }; }
static v4 v4_zwxy(v4 v) { return (v4) { v.z, v.w, v.x, v.y }; }
static v4 v4_zwxz(v4 v) { return (v4) { v.z, v.w, v.x, v.z }; }
static v4 v4_zwxw(v4 v) { return (v4) { v.z, v.w, v.x, v.w }; }
static v4 v4_zwyx(v4 v) { return (v4) { v.z, v.w, v.y, v.x }; }
static v4 v4_zwyy(v4 v) { return (v4) { v.z, v.w, v.y, v.y }; }
static v4 v4_zwyz(v4 v) { return (v4) { v.z, v.w, v.y, v.z }; }
static v4 v4_zwyw(v4 v) { return (v4) { v.z, v.w, v.y, v.w }; }
static v4 v4_zwzx(v4 v) { return (v4) { v.z, v.w, v.z, v.x }; }
static v4 v4_zwzy(v4 v) { return (v4) { v.z, v.w, v.z, v.y }; }
static v4 v4_zwzz(v4 v) { return (v4) { v.z, v.w, v.z, v.z }; }
static v4 v4_zwzw(v4 v) { return (v4) { v.z, v.w, v.z, v.w }; }
static v4 v4_zwwx(v4 v) { return (v4) { v.z, v.w, v.w, v.x }; }
static v4 v4_zwwy(v4 v) { return (v4) { v.z, v.w, v.w, v.y }; }
static v4 v4_zwwz(v4 v) { return (v4) { v.z, v.w, v.w, v.z }; }
static v4 v4_zwww(v4 v) { return (v4) { v.z, v.w, v.w, v.w }; }

static v4 v4_wxxx(v4 v) { return (v4) { v.w, v.x, v.x, v.x }; }
static v4 v4_wxxy(v4 v) { return (v4) { v.w, v.x, v.x, v.y }; }
static v4 v4_wxxz(v4 v) { return (v4) { v.w, v.x, v.x, v.z }; }
static v4 v4_wxxw(v4 v) { return (v4) { v.w, v.x, v.x, v.w }; }
static v4 v4_wxyx(v4 v) { return (v4) { v.w, v.x, v.y, v.x }; }
static v4 v4_wxyy(v4 v) { return (v4) { v.w, v.x, v.y, v.y }; }
static v4 v4_wxyz(v4 v) { return (v4) { v.w, v.x, v.y, v.z }; }
static v4 v4_wxyw(v4 v) { return (v4) { v.w, v.x, v.y, v.w }; }
static v4 v4_wxzx(v4 v) { return (v4) { v.w, v.x, v.z, v.x }; }
static v4 v4_wxzy(v4 v) { return (v4) { v.w, v.x, v.z, v.y }; }
static v4 v4_wxzz(v4 v) { return (v4) { v.w, v.x, v.z, v.z }; }
static v4 v4_wxzw(v4 v) { return (v4) { v.w, v.x, v.z, v.w }; }
static v4 v4_wxwx(v4 v) { return (v4) { v.w, v.x, v.w, v.x }; }
static v4 v4_wxwy(v4 v) { return (v4) { v.w, v.x, v.w, v.y }; }
static v4 v4_wxwz(v4 v) { return (v4) { v.w, v.x, v.w, v.z }; }
static v4 v4_wxww(v4 v) { return (v4) { v.w, v.x, v.w, v.w }; }
static v4 v4_wyxx(v4 v) { return (v4) { v.w, v.y, v.x, v.x }; }
static v4 v4_wyxy(v4 v) { return (v4) { v.w, v.y, v.x, v.y }; }
static v4 v4_wyxz(v4 v) { return (v4) { v.w, v.y, v.x, v.z }; }
static v4 v4_wyxw(v4 v) { return (v4) { v.w, v.y, v.x, v.w }; }
static v4 v4_wyyx(v4 v) { return (v4) { v.w, v.y, v.y, v.x }; }
static v4 v4_wyyy(v4 v) { return (v4) { v.w, v.y, v.y, v.y }; }
static v4 v4_wyyz(v4 v) { return (v4) { v.w, v.y, v.y, v.z }; }
static v4 v4_wyyw(v4 v) { return (v4) { v.w, v.y, v.y, v.w }; }
static v4 v4_wyzx(v4 v) { return (v4) { v.w, v.y, v.z, v.x }; }
static v4 v4_wyzy(v4 v) { return (v4) { v.w, v.y, v.z, v.y }; }
static v4 v4_wyzz(v4 v) { return (v4) { v.w, v.y, v.z, v.z }; }
static v4 v4_wyzw(v4 v) { return (v4) { v.w, v.y, v.z, v.w }; }
static v4 v4_wywx(v4 v) { return (v4) { v.w, v.y, v.w, v.x }; }
static v4 v4_wywy(v4 v) { return (v4) { v.w, v.y, v.w, v.y }; }
static v4 v4_wywz(v4 v) { return (v4) { v.w, v.y, v.w, v.z }; }
static v4 v4_wyww(v4 v) { return (v4) { v.w, v.y, v.w, v.w }; }
static v4 v4_wzxx(v4 v) { return (v4) { v.w, v.z, v.x, v.x }; }
static v4 v4_wzxy(v4 v) { return (v4) { v.w, v.z, v.x, v.y }; }
static v4 v4_wzxz(v4 v) { return (v4) { v.w, v.z, v.x, v.z }; }
static v4 v4_wzxw(v4 v) { return (v4) { v.w, v.z, v.x, v.w }; }
static v4 v4_wzyx(v4 v) { return (v4) { v.w, v.z, v.y, v.x }; }
static v4 v4_wzyy(v4 v) { return (v4) { v.w, v.z, v.y, v.y }; }
static v4 v4_wzyz(v4 v) { return (v4) { v.w, v.z, v.y, v.z }; }
static v4 v4_wzyw(v4 v) { return (v4) { v.w, v.z, v.y, v.w }; }
static v4 v4_wzzx(v4 v) { return (v4) { v.w, v.z, v.z, v.x }; }
static v4 v4_wzzy(v4 v) { return (v4) { v.w, v.z, v.z, v.y }; }
static v4 v4_wzzz(v4 v) { return (v4) { v.w, v.z, v.z, v.z }; }
static v4 v4_wzzw(v4 v) { return (v4) { v.w, v.z, v.z, v.w }; }
static v4 v4_wzwx(v4 v) { return (v4) { v.w, v.z, v.w, v.x }; }
static v4 v4_wzwy(v4 v) { return (v4) { v.w, v.z, v.w, v.y }; }
static v4 v4_wzwz(v4 v) { return (v4) { v.w, v.z, v.w, v.z }; }
static v4 v4_wzww(v4 v) { return (v4) { v.w, v.z, v.w, v.w }; }
static v4 v4_wwxx(v4 v) { return (v4) { v.w, v.w, v.x, v.x }; }
static v4 v4_wwxy(v4 v) { return (v4) { v.w, v.w, v.x, v.y }; }
static v4 v4_wwxz(v4 v) { return (v4) { v.w, v.w, v.x, v.z }; }
static v4 v4_wwxw(v4 v) { return (v4) { v.w, v.w, v.x, v.w }; }
static v4 v4_wwyx(v4 v) { return (v4) { v.w, v.w, v.y, v.x }; }
static v4 v4_wwyy(v4 v) { return (v4) { v.w, v.w, v.y, v.y }; }
static v4 v4_wwyz(v4 v) { return (v4) { v.w, v.w, v.y, v.z }; }
static v4 v4_wwyw(v4 v) { return (v4) { v.w, v.w, v.y, v.w }; }
static v4 v4_wwzx(v4 v) { return (v4) { v.w, v.w, v.z, v.x }; }
static v4 v4_wwzy(v4 v) { return (v4) { v.w, v.w, v.z, v.y }; }
static v4 v4_wwzz(v4 v) { return (v4) { v.w, v.w, v.z, v.z }; }
static v4 v4_wwzw(v4 v) { return (v4) { v.w, v.w, v.z, v.w }; }
static v4 v4_wwwx(v4 v) { return (v4) { v.w, v.w, v.w, v.x }; }
static v4 v4_wwwy(v4 v) { return (v4) { v.w, v.w, v.w, v.y }; }
static v4 v4_wwwz(v4 v) { return (v4) { v.w, v.w, v.w, v.z }; }
static v4 v4_wwww(v4 v) { return (v4) { v.w, v.w, v.w, v.w }; }

#endif
