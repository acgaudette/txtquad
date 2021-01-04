#version 450
#include "config.h"
#define SCALE (float(CHAR_WIDTH) / FONT_WIDTH)

#define VERT_MIN (0.f - PADDING)
#define VERT_MAX (1.f + PADDING)
#define   SQ_MIN (MIN_BIAS - PADDING)
#define   SQ_MAX (MAX_BIAS + PADDING)

struct Char {
	mat4 model;
	vec4 col;
	vec2 off;
	vec2 fx;
};

layout (set = 1, binding = 0) uniform Share {
	mat4 vp;
	vec2 screen;
	float time;
} share;

layout (set = 2, binding = 0) readonly buffer Data { Char chars[MAX_CHAR]; } data;

#ifdef PLATFORM_COMPAT_VBO
	layout (location = 0) in vec2 pos;
	layout (location = 1) in vec2 sq;
#else
const vec4 vert[4] = {
	  vec4(VERT_MIN, VERT_MAX, 0, 1)
	, vec4(VERT_MAX, VERT_MAX, 0, 1)
	, vec4(VERT_MIN, VERT_MIN, 0, 1)
	, vec4(VERT_MAX, VERT_MIN, 0, 1)
};

const vec2 sq[4] = {
	  vec2(SQ_MIN, SQ_MIN)
	, vec2(SQ_MAX, SQ_MIN)
	, vec2(SQ_MIN, SQ_MAX)
	, vec2(SQ_MAX, SQ_MAX)
};
#endif

layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 st;
layout (location = 2) out vec4 col;
layout (location = 3) out vec2 fx;
layout (location = 4) out vec3 pos;

void main()
{
	Char c = data.chars[gl_InstanceIndex];

#ifdef PLATFORM_COMPAT_VBO
	st = sq;
#else
	st = sq[gl_VertexIndex];
#endif

	uv = SCALE * (st + c.off);
	col = c.col;
	fx = c.fx;

#ifdef PLATFORM_COMPAT_VBO
	vec4 world = c.model * vec4(pos, 0, 1);
#else
	vec4 world = c.model * vert[gl_VertexIndex];
#endif
	gl_Position = share.vp * world;
	pos = world.xyz;
}
