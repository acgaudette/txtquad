#version 450

layout (location = 0) in  vec2 uv;
layout (location = 0) out vec4 color;

layout (set = 0, binding = 0) uniform texture2D img;
layout (set = 0, binding = 1) uniform sampler unf;

void main()
{
	float b = texture(sampler2D(img, unf), uv).r;
	color = vec4(b);
}
