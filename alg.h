#ifndef ALG_H
#define ALG_H

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

#define MAT(N, ...) typedef union m ## N { \
        float s[N * N];                    \
        v ## N v[N];                       \
        struct { __VA_ARGS__ };            \
} m ## N

#define VEC_2 v4 c0; \
              v4 c1;
#define VEC_3 VEC_2 v4 c2;
#define VEC_4 VEC_3 v4 c3;

MAT(2, VEC_2);
MAT(3, VEC_3);
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

static m4 m4_model(v3 pos, float scale)
{
	return m4_mul(m4_trans(pos), m4_scale(scale));
}

// perspective (easy)
// quat / mat conversion (later)

#include <stdio.h>
#define PRINT(N) static void v ## N ## _print(v ## N v) \
{ \
	for (size_t i = 0; i < N; ++i) \
		printf("%8.2f  ", v.s[i]); \
	printf("\n"); \
}

PRINT(2)
PRINT(3)
PRINT(4)
#undef PRINT

#define PRINT(N) static void m ## N ## _print(m ## N m) \
{ \
	for (size_t i = 0; i < N; ++i) { \
		for (size_t j = 0; j < N; ++j) { \
			printf("%8.2f  ", m.v[j].s[i]); \
		} \
		printf("\n"); \
	} \
}

PRINT(2)
PRINT(3)
PRINT(4)
#undef PRINT

#endif
