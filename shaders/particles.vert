#version 150 core

in vec2 pos;

out vec3 frag_color;
out vec2 out_position;

uniform vec3 vert_color;
uniform vec2 mouse_pos;

void main()
{
	vec2 mdist = mouse_pos - pos;
	vec2 mdir = mdist / length(mdist);
	out_position = pos + 0.01 * mdir;
	gl_Position = vec4(out_position, 0.0, 1.0);
	frag_color = vert_color;
}
