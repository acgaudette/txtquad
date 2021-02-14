#ifndef ALG_H
#define ALG_H

#define _USE_MATH_DEFINES
#include <math.h>
#ifdef ALG_DEBUG
#include <assert.h>
#endif
#include "types.h"

#define VEC(N, ...) typedef union v ## N { \
        float s[N];                        \
        struct { __VA_ARGS__ };            \
} v ## N

#define COMP_2 float x; \
               float y;
#define COMP_3 COMP_2 float z;
#define COMP_4 COMP_3 float w;

VEC(2, COMP_2);
VEC(3, COMP_3);
VEC(4, COMP_4);
#undef VEC

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

#define FILL(N) static v ## N v ## N ## _fill(float s) \
{ \
	v ## N v; \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] = s; \
	return v; \
}

FILL(2)
FILL(3)
FILL(4)
#undef FILL

#define SHIFT(N) static v ## N v ## N ## _shift(v ## N v, const u8 i) \
{ \
	v ## N swap = v; \
	for (u8 j = 0; j < N; ++j) { \
		u8 k = (i + j) % N; \
		v.s[j] = swap.s[k]; \
	} \
	return v; \
}

SHIFT(2)
SHIFT(3)
SHIFT(4)
#undef SHIFT

#define ZERO(N) static v ## N v ## N ## _zero() \
{ \
	return v ## N ## _fill(0.f); \
}

ZERO(2)
ZERO(3)
ZERO(4)
#undef ZERO

#define ONE(N) static v ## N v ## N ## _one() \
{ \
	return v ## N ## _fill(1.f); \
}

ONE(2)
ONE(3)
ONE(4)
#undef ONE

#define RIGHT(N) static v ## N v ## N ## _right() \
{ \
	v ## N v = v ## N ## _zero(); \
	v.s[0] = 1.f; \
	return v; \
}

RIGHT(2)
RIGHT(3)
RIGHT(4)
#undef RIGHT

#define UP(N) static v ## N v ## N ## _up() \
{ \
	v ## N v = v ## N ## _zero(); \
	v.s[1] = 1.f; \
	return v; \
}

UP(2)
UP(3)
UP(4)
#undef UP

#define FWD(N) static v ## N v ## N ## _fwd() \
{ \
	v ## N v = v ## N ## _zero(); \
	v.s[2] = 1.f; \
	return v; \
}

FWD(3)
FWD(4)
#undef FWD

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

#define NEG(N) static v ## N v ## N ## _neg(v ## N v) \
{ \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] *= -1.f; \
	return v; \
}

NEG(2)
NEG(3)
NEG(4)
#undef NEG

#define ADD(N) static v ## N v ## N ## _add(v ## N a, v ## N b) \
{ \
	for (size_t i = 0; i < N; ++i) \
		a.s[i] += b.s[i]; \
	return a; \
}

ADD(2)
ADD(3)
ADD(4)
#undef ADD

#define ADDEQ(N) static void v ## N ## _addeq(v ## N *a, v ## N b) \
{ \
	*a = v ## N ## _add(*a, b); \
}

ADDEQ(2)
ADDEQ(3)
ADDEQ(4)
#undef ADD_EQ

#define SUB(N) static v ## N v ## N ## _sub(v ## N a, v ## N b) \
{ \
	for (size_t i = 0; i < N; ++i) \
		a.s[i] -= b.s[i]; \
	return a; \
}

SUB(2)
SUB(3)
SUB(4)
#undef SUB

#define MUL(N) static v ## N v ## N ## _mul(v ## N v, float s) \
{ \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] *= s; \
	return v; \
}

MUL(2)
MUL(3)
MUL(4)
#undef MUL

#define SCHUR(N) static v ## N v ## N ## _schur(v ## N a, v ## N b) \
{ \
	for (size_t i = 0; i < N; ++i) \
		a.s[i] *= b.s[i]; \
	return a; \
}

SCHUR(2)
SCHUR(3)
SCHUR(4)
#undef SCHUR

#define MAG_SQ(N) static float v ## N ## _mag_sq(v ## N v) \
{ \
	float s = 0; \
	for (size_t i = 0; i < N; ++i) \
		s += v.s[i] * v.s[i]; \
	return s; \
}

MAG_SQ(2)
MAG_SQ(3)
MAG_SQ(4)
#undef MAG_SQ

#define MAG(N) static float v ## N ## _mag(v ## N v) \
{ \
	return sqrt(v ## N ## _mag_sq(v)); \
}

MAG(2)
MAG(3)
MAG(4)
#undef MAG

// TODO: verify nonzero
#define NORM(N) static v ## N v ## N ## _norm(v ## N v) \
{ \
	float inv = 1.f / v ## N ## _mag(v); \
	for (size_t i = 0; i < N; ++i) \
		v.s[i] *= inv; \
	return v; \
}

NORM(2)
NORM(3)
NORM(4)
#undef NORM

#define IS_NORM(N) static int v ## N ## _is_norm(v ## N v) \
{ \
	float mag_sq = v ## N ## _mag_sq(v); \
	return fabsf(1.f - mag_sq) < 1e-6; \
}

IS_NORM(2)
IS_NORM(3)
IS_NORM(4)
#undef IS_NORM

#define DOT(N) static float v ## N ## _dot(v ## N a, v ## N b) \
{                                                              \
        float s = 0;                                           \
        for (size_t i = 0; i < N; ++i)                         \
                s += a.s[i] * b.s[i];                          \
        return s;                                              \
}

DOT(2)
DOT(3)
DOT(4)
#undef DOT

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

#define LERP_CLAMP(N) static v ## N v ## N ## _lerp_clamp(v ## N a, v ## N b, float s) \
{ \
	return v ## N ## _lerp(a, b, clamp01f(s)); \
}

LERP_CLAMP(2)
LERP_CLAMP(3)
LERP_CLAMP(4)
#undef LERP_CLAMP

#define DIST(N) static float v ## N ## _dist(v ## N a, v ## N b) \
{ \
	return v ## N ## _mag(v ## N ## _sub(a, b)); \
}

DIST(2)
DIST(3)
DIST(4)
#undef DIST

static v3 v3_cross(v3 a, v3 b)
{
	return (v3) {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

/* Matrices */

#define ZERO(N) static m ## N m ## N ## _zero() \
{ \
	m ## N m; \
	for (size_t i = 0; i < N; ++i) \
		m.v[i] = v ## N ## _zero(); \
	return m; \
}

ZERO(2)
ZERO(3)
ZERO(4)
#undef ZERO

#define ID(N) static m ## N m ## N ## _id() \
{ \
	m ## N m = m ## N ## _zero(); \
	for (size_t i = 0; i < N; ++i) \
		m.s[i + N * i] = 1.f; \
	return m; \
}

ID(2)
ID(3)
ID(4)
#undef ID

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
		for (size_t j = 0; j < N; ++j) { \
			m.v[i].s[j] = v ## N ## _dot( \
				m ## N ## _row(a, j), \
				b.v[i] \
			); \
		} \
	} \
	return m; \
}

MUL(2)
MUL(3)
MUL(4)
#undef MUL

#define DIAG(N) static m ## N m ## N ## _diag(v ## N v) \
{ \
	m ## N m = m ## N ## _zero(); \
	for (size_t i = 0; i < N; ++i) \
		m.s[i + N * i] = v.s[i]; \
	return m; \
}

DIAG(2);
DIAG(3);
DIAG(4);
#undef DIAG

#define FILL(M, N) static m ## M m ## M ## _fill_m ## N (m ## N n) \
{ \
	m ## M m = m ## M ## _zero(); \
	for (size_t i = 0; i < N; ++i) { \
		for (size_t j = 0; j < N; ++j) { \
			m.v[i].s[j] = n.v[i].s[j]; \
		} \
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

static m4 m4_trans(v3 v)
{
	m4 m = m4_id();
	m.c3.x = v.x;
	m.c3.y = v.y;
	m.c3.z = v.z;
	return m;
}

static m4 m4_scale(float s)
{
	return m4_diag((v4) { s, s, s, 1.f });
}

static m4 m4_scale_aniso(float x, float y, float z)
{
	return m4_diag((v4) { x, y, z, 1.f });
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

	m4 m = m4_zero();
	m.c0.x = x_scale;
	m.c1.y = y_scale;
	m.c2.z = z_scale;
	m.c2.w = 1.f;
	m.c3.z = z_off;
	return m;
}

// Scale -> number of units that can fit in vertical height
static m4 m4_ortho(float scale, float asp, float near, float far)
{
	const float range = far - near;
	const float z_scale = 1.f / range;
	const float z_off = -near / range;

	m4 m = m4_zero();
	m.c0.x = 2.f / (asp * scale);
	m.c1.y = -2.f / scale;
	m.c2.z = z_scale;
	m.c3.w = 1.f;
	m.c3.z = z_off;
	return m;
}

/* Quaternions */

static inline v4 qt_id()
{
	return (v4) { 0.f, 0.f, 0.f, 1.f };
}

static inline v4 qt_conj(v4 q)
{
	return (v4) { -q.x, -q.y, -q.z, q.w, };
}

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

// Assume fwd and up are not orthogonal
// (o/w should convert from m3 directly)
static v4 qt_vecs_up(v3 fwd, v3 up)
{
	// assert(v3_is_norm(fwd));
	// assert(v3_is_norm(up));

	v3 right = v3_cross(up, fwd);
	right = v3_norm(right); // TODO: if not already orthogonal
	fwd = v3_cross(right, up);

	m3 m;
	m.c0 = right;
	m.c1 = up;
	m.c2 = fwd;

	return m3_to_qt(m);
}

static v4 qt_vecs_fwd(v3 fwd, v3 up)
{
	// assert(v3_is_norm(fwd));
	// assert(v3_is_norm(up));

	v3 right = v3_cross(up, fwd);
	right = v3_norm(right); // TODO: if not already orthogonal
	up = v3_cross(fwd, right);

	m3 m;
	m.c0 = right;
	m.c1 = up;
	m.c2 = fwd;

	return m3_to_qt(m);
}

static v4 qt_axis_angle(v3 axis, float angle)
{
#ifdef ALG_DEBUG
	assert(v3_is_norm(axis));
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

// TODO
// gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
static v3 qt_app(v4 q, v3 v)
{
	v3 b = (v3) { q.x, q.y, q.z };
	float s = q.w;
	return v3_add(
		v3_mul(b, 2.f * v3_dot(v, b)),
		v3_add(
			v3_mul(v, s * s - v3_dot(b, b)),
			v3_mul(v3_cross(b, v), 2.f * s)
		)
	);
}

// TODO: hamilton (corpus/edu/math/linear)
static m3 qt_to_m3(v4 q)
{
	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float ww = q.w * q.w;
	float inv = 1.f / (xx + yy + zz + ww);

	float x = inv * ( xx - yy - zz + ww);
	float y = inv * (-xx + yy - zz + ww);
	float z = inv * (-xx - yy + zz + ww);
	m3 m = m3_diag((v3) { x, y, z });

	float _2inv = 2.f * inv;
	{
		float xy = q.x * q.y;
		float zw = q.z * q.w;
		m.c0.y = _2inv * (xy + zw);
		m.c1.x = _2inv * (xy - zw);
	} {
		float xz = q.x * q.z;
		float yw = q.y * q.w;
		m.c0.z = _2inv * (xz - yw);
		m.c2.x = _2inv * (xz + yw);
	} {
		float yz = q.y * q.z;
		float xw = q.x * q.w;
		m.c1.z = _2inv * (yz + xw);
		m.c2.y = _2inv * (yz - xw);
	}

	return m;
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
	return m4_mul(m4_trans(pos), m4_mul(qt_to_m4(rot), m4_scale(scale)));
}

static m4 m4_view(v3 pos, v4 rot)
{
	return m4_mul(qt_to_m4(qt_conj(rot)), m4_trans(v3_neg(pos)));
}

/* Debug */

#include <stdio.h>
#define FPRINT(N) static void v ## N ## _fprint(FILE *stream, v ## N v) \
{ \
	for (size_t i = 0; i < N; ++i) \
		fprintf(stream, "%8.4f  ", v.s[i]); \
	fprintf(stream, "\n"); \
}

FPRINT(2)
FPRINT(3)
FPRINT(4)
#undef FPRINT

#define PRINT(N) static void v ## N ## _print(v ## N v) \
{ \
	v ## N ## _fprint(stdout, v); \
}

PRINT(2)
PRINT(3)
PRINT(4)
#undef PRINT

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
