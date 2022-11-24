#version 300 es

uniform sampler2D inputs;

out mediump vec4 out_sum;

void main()
{
	// Hard-code fold factor of 4 to avoid a loop
	ivec2 sum_base = ivec2(int(gl_FragCoord.x) * 4, int(gl_FragCoord.y));
	mediump vec4 a = texelFetch(inputs, sum_base + ivec2(0, 0), 0);
	mediump vec4 b = texelFetch(inputs, sum_base + ivec2(1, 0), 0);
	mediump vec4 c = texelFetch(inputs, sum_base + ivec2(2, 0), 0);
	mediump vec4 d = texelFetch(inputs, sum_base + ivec2(3, 0), 0);

	out_sum = a + b + c + d;
}
