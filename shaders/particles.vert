#version 300 es

in vec2 vert_displacement;
in vec4 color;

uniform sampler2D positions;

out vec4 frag_color;

const float planet_r = 0.02; // Radius of each planet relative to screen

void main()
{
	vec2 current_pos = texelFetch(positions, ivec2(gl_InstanceID, 0), 0).xy;
	gl_Position = vec4(current_pos + planet_r * vert_displacement, 0.0, 1.0);
	frag_color = color;
}
