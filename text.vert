#version 450

layout (location = 0) out vec2 uv;

// TODO: generate via math?

const vec2 vert[4] = {
	  vec2(-1, -1)
	, vec2( 1, -1)
	, vec2(-1,  1)
	, vec2( 1,  1)
};

const vec2 sq[4] = {
	  vec2(0, 0)
	, vec2(1, 0)
	, vec2(0, 1)
	, vec2(1, 1)
};

#define WIDTH 128
#define SIZE 8
#define     SCALE (float(SIZE) / WIDTH)
#define INV_SCALE (WIDTH / SIZE)

int char = 65;

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

	uv = SCALE * (
		// TODO: probably slow and should be done on the cpu
		sq[gl_VertexIndex] + vec2(char % INV_SCALE, char / INV_SCALE)
	);

	gl_Position = vec4(vert[gl_VertexIndex], 0, 1);
}
