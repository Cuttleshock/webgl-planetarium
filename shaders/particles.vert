#version 150 core

in vec2 pos;

out vec3 frag_color;
out vec2 out_position;

uniform vec3 vert_color;

void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);
	out_position = pos - 0.01;
	frag_color = vert_color;
}
