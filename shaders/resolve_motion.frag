#version 300 es

in mediump vec2 speed_feedback;

uniform sampler2D positions;

out mediump vec4 out_position;

void main()
{
	mediump vec4 my_pos = texelFetch(positions, ivec2(int(gl_FragCoord.x), 0), 0);

	out_position = vec4(my_pos.xy + speed_feedback, speed_feedback);
}
