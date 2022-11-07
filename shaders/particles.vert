#version 150 core

in vec2 home_pos;
in vec2 current_pos;

out vec3 frag_color;
out vec2 home_pos_feedback;
out vec2 current_pos_feedback;

uniform vec3 vert_color;
uniform vec2 mouse_pos;

void main()
{
	vec2 target = 0.5 * (home_pos + mouse_pos);
	vec2 to_target = target - current_pos;
	vec2 direction = to_target / length(to_target);
	current_pos_feedback = current_pos + 0.01 * direction;
	home_pos_feedback = home_pos;
	gl_Position = vec4(current_pos_feedback, 0.0, 1.0);
	frag_color = vert_color;
}
