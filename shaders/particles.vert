#version 300 es

in vec2 current_pos;
in vec2 vert_displacement;
in vec4 color;

out vec4 frag_color;

const float planet_r = 0.02; // Radius of each planet relative to screen

void main()
{
	gl_Position = vec4(current_pos + planet_r * vert_displacement, 0.0, 1.0);
	frag_color = color;
}
