#version 450

// TODO: inject
#define MAX_CHAR 4096

// TODO: pass in as ubo (or inject)
#define FONT_WIDTH 128
#define GLYPH_WIDTH 8
#define SCALE (float(GLYPH_WIDTH) / FONT_WIDTH)

struct Char {
	mat4 model;
	vec4 col;
	vec2 off;
};

layout (set = 1, binding = 0) uniform Data {
	Char chars[MAX_CHAR];
} data;
layout (location = 0) out vec2 uv;

// TODO: generate via math?

const vec4 vert[4] = {
	  vec4(-.5f, -.5f, 0, 1)
	, vec4( .5f, -.5f, 0, 1)
	, vec4(-.5f,  .5f, 0, 1)
	, vec4( .5f,  .5f, 0, 1)
};

const vec2 sq[4] = {
	  vec2(0, 0)
	, vec2(1, 0)
	, vec2(0, 1)
	, vec2(1, 1)
};

void main()
{
	/*
	uv = vec2(
		(gl_VertexIndex << 1) & 2,
		 gl_VertexIndex & 2
	);

	gl_Position = vec4(
		2 * uv - 1,
		0,
		1
	);
	*/

	Char char = data.chars[gl_InstanceIndex];
	uv = SCALE * (sq[gl_VertexIndex] + char.off);
	gl_Position = char.model * vert[gl_VertexIndex];
}
