#version 450
#include "config.h"
#define SCALE (float(CHAR_WIDTH) / FONT_WIDTH)

struct Char {
	mat4 model;
	vec4 col;
	vec2 off;
};

layout (set = 1, binding = 0) uniform Share { mat4 vp; } share;
layout (set = 2, binding = 0) uniform Data { Char chars[MAX_CHAR]; } data;
layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 st;
layout (location = 2) out vec4 col;

const vec4 vert[4] = {
	  vec4(0, 1, 0, 1)
	, vec4(1, 1, 0, 1)
	, vec4(0, 0, 0, 1)
	, vec4(1, 0, 0, 1)
};

const vec2 sq[4] = {
	  vec2(0, 0)
	, vec2(1, 0)
	, vec2(0, 1)
	, vec2(1, 1)
};

void main()
{
	Char c = data.chars[gl_InstanceIndex];
	st = sq[gl_VertexIndex];
	uv = SCALE * (st + c.off);
	col = c.col;
	gl_Position = share.vp * c.model * vert[gl_VertexIndex];
}
