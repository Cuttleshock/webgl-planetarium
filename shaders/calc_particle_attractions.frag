#version 300 es

uniform sampler2D positions;
uniform mediump float planet_r;

out mediump vec2 out_attraction;

const mediump float gravitational_constant = 0.01;

void main()
{
	mediump vec4 my_pos = texelFetch(positions, ivec2(int(gl_FragCoord.x), 0), 0);
	mediump vec4 your_pos = texelFetch(positions, ivec2(int(gl_FragCoord.y), 0), 0);

	mediump vec2 separation = (my_pos - your_pos).xy;

	mediump float divisor = max(length(separation), planet_r);
	out_attraction = separation * gravitational_constant / (divisor * divisor * divisor);
}
