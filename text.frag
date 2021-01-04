#version 450

/* Example txtquad fragment shader */

layout (location = 0) in  vec2 uv;
layout (location = 1) in  vec2 st;
layout (location = 2) in  vec4 col;
layout (location = 3) in  vec2 fx;
layout (location = 0) out vec4 final;

layout (set = 0, binding = 0) uniform texture2D img;
layout (set = 0, binding = 1) uniform sampler unf;

void main()
{
	if (min(st.x, st.y) < 0 || max(st.x, st.y) > 1) discard; // Padding
	float b = texture(sampler2D(img, unf), uv).r;
	if (0 == b) discard;

	if (col.a < st.y) discard; // e.g. basic wipe effect
	final = vec4(b * col.rgb, 1);
}
