#version 150 core

in vec2 home_pos;
in vec2 current_pos;

out vec3 frag_color;
out vec2 home_pos_feedback;
out vec2 current_pos_feedback;

uniform vec3 vert_color;
uniform vec2 mouse_pos;

const float r = 0.2; // Radius beyond which particle will not go
const float speed = 0.01; // Distance point moves per frame
const float g = 2.0; // Relative strength of home vs mouse (1.0 - inf)

void main()
{
	vec2 home_to_mouse_v = mouse_pos - home_pos;
	float home_to_mouse_d = length(home_to_mouse_v);
	vec2 target = (1.0 / g) * mouse_pos + (1.0 - 1.0 / g) * home_pos;
	if (home_to_mouse_d > r * g) {
		target = home_pos + home_to_mouse_v * (r * r * g) / (home_to_mouse_d * home_to_mouse_d);
	}
	vec2 to_target = target - current_pos;
	vec2 direction = to_target / length(to_target);
	if (length(to_target) > speed) {
		current_pos_feedback = current_pos + speed * direction;
	} else {
		current_pos_feedback = target;
	}
	home_pos_feedback = home_pos;
	gl_Position = vec4(current_pos_feedback, 0.0, 1.0);
	frag_color = vert_color;
}
