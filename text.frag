#version 450

/* Example txtquad fragment shader */

layout (location = 0) in  vec2 uv;
layout (location = 1) in  vec2 st;
layout (location = 2) in  vec4 col;
layout (location = 3) in  vec2 fx;
layout (location = 4) in  vec3 pos;
layout (location = 0) out vec4 final;

layout (set = 1, binding = 0) uniform Share {
	mat4 vp;
	vec2 screen;
	float time;
} share;

layout (set = 0, binding = 0) uniform texture2D img;
layout (set = 0, binding = 1) uniform sampler unf;

void main()
{
	if (min(st.x, st.y) < 0 || max(st.x, st.y) > 1) discard; // Padding
	float b = texture(sampler2D(img, unf), uv).r;
	if (0 == b) discard;

	if ((1 - col.a) > 2 * abs(st.y - .5)) discard; // e.g. basic wipe effect
	final = vec4(b * col.rgb, 1);
}
