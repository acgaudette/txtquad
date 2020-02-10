#version 450

layout (location = 0) in  vec2 uv;
layout (location = 1) in  vec2 st;
layout (location = 2) in  vec4 col;
layout (location = 0) out vec4 final;

layout (set = 0, binding = 0) uniform texture2D img;
layout (set = 0, binding = 1) uniform sampler unf;

float noise()
{
	float scale = 128.0;
	vec2 base = floor(uv * scale);

	float x = 516279. * base.x + 314523. * base.y;
	x = mod(x, 2. * 3.14159);
	float raw = sin(x) * 487915.0;

	return fract(raw);
}

void main()
{
	float b = texture(sampler2D(img, unf), uv).r;
	// if (b == 0 || col.a < st.y) discard; // Wipe
	   if (b == 0 || col.a == 0 || col.a < noise()) discard; // Dissolve
	final = vec4(b * col.rgb, 1);
}
