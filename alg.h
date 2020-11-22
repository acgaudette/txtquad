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
	return fabsf(1.f - mag_sq) < 0.000001f; \
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

static m4 m4_model(v3 pos, v4 rot, float scale)
{
	return m4_mul(m4_trans(pos), m4_mul(qt_to_m4(rot), m4_scale(scale)));
}

static m4 m4_view(v3 pos, v4 rot)
{
	return m4_mul(qt_to_m4(qt_conj(rot)), m4_trans(v3_neg(pos)));
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

/* Floating point functions */

static inline float lerpf(const float a, const float b, const float t)
{
	return (1.f - t) * a + t * b;
}

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

static inline float signf(const float s)
{
	u32 u = *((u32*)&s);
	return (float)(u >> 31) * -2.f + 1.f;
}

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

#endif
