#version 300 es

in vec2 home_pos;
in vec2 speed;
in vec2 current_pos;

out vec3 frag_color;

out vec2 home_pos_feedback;
out vec2 speed_feedback;
out vec2 current_pos_feedback;

uniform vec3 vert_color;
uniform vec2 mouse_pos;

const float home_m = 1.0; // Mass of home well
const float home_r = 0.2; // Radius (below which gravity is constant).
// Strictly it should be linear, but that makes the centre of gravity unstable.
const float mouse_m = 0.01;
const float mouse_r = 0.1;
const float time_step = 0.0001; // For applying accel to speed.
// Speed is then treated as time-normalised.
const float damping = 0.98; // Prevent the system from accumulating energy

void main()
{
	vec2 to_home = home_pos - current_pos;
	float to_home_r = length(to_home);
	vec2 to_mouse = mouse_pos - current_pos;
	float to_mouse_r = length(to_mouse);
	vec2 accel = vec2(0.0);

	if (to_home_r >= home_r) {
		accel += to_home * home_m / (to_home_r * to_home_r * to_home_r);
	} else if (to_home_r > 0.0) {
		accel += to_home * home_m / (to_home_r * home_r * home_r);
	}

	if (to_mouse_r >= mouse_r) {
		accel += to_mouse * mouse_m / (to_mouse_r * to_mouse_r * to_mouse_r);
	} else if (to_mouse_r > 0.0) {
		accel += to_mouse * mouse_m / (to_mouse_r * mouse_r * mouse_r);
	}

	speed_feedback = (speed + accel * time_step) * damping;
	current_pos_feedback = current_pos + speed;
	home_pos_feedback = home_pos;
	gl_Position = vec4(current_pos_feedback, 0.0, 1.0);
	frag_color = vert_color;
}
