#version 450
#include "config.h"
#define SCALE (float(CHAR_WIDTH) / FONT_WIDTH)

struct Char {
	mat4 model;
	vec4 col;
	vec2 off;
};

layout (set = 1, binding = 0) uniform Share { mat4 vp; } share;
layout (set = 2, binding = 0) readonly buffer Data { Char chars[MAX_CHAR]; } data;

#ifdef PLATFORM_COMPAT_VBO
	layout (location = 0) in vec2 pos;
	layout (location = 1) in vec2 sq;
#else
const vec4 vert[4] = {
	  vec4(0, 1, 0, 1)
	, vec4(1, 1, 0, 1)
	, vec4(0, 0, 0, 1)
	, vec4(1, 0, 0, 1)
};

const vec2 sq[4] = {
	  vec2(MIN_BIAS, MIN_BIAS)
	, vec2(MAX_BIAS, MIN_BIAS)
	, vec2(MIN_BIAS, MAX_BIAS)
	, vec2(MAX_BIAS, MAX_BIAS)
};
#endif

layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 st;
layout (location = 2) out vec4 col;

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

#ifdef PLATFORM_COMPAT_VBO
	gl_Position = share.vp * c.model * vec4(pos, 0, 1);
#else
	gl_Position = share.vp * c.model * vert[gl_VertexIndex];
#endif
}
