#version 300 es

in vec2 speed;
in vec4 color;

out vec2 speed_feedback;
out vec4 color_feedback;

uniform vec2 mouse_pos;
uniform float num_planets;
uniform sampler2D positions;

const float mouse_m = 0.5; // Mass of mouse gravity well
const float mouse_r = 0.1; // Radius (below which gravity is ignored, preventing accumulation)
const float time_step = 0.0001; // For applying accel to speed.
// Speed is then treated as time-normalised.
const float damping = 0.99; // Prevent the system from accumulating energy

void main()
{
	vec2 current_pos = texelFetch(positions, ivec2(gl_VertexID, 0), 0).xy;

	vec2 to_mouse = mouse_pos - current_pos;
	float to_mouse_r = length(to_mouse);
	vec2 to_mouse_dir = to_mouse / to_mouse_r;
	vec2 accel = vec2(0.0);

	if (to_mouse_r >= mouse_r) {
		accel += to_mouse_dir * mouse_m / (to_mouse_r * to_mouse_r);
	}

	speed_feedback = (speed + accel * time_step) * damping;
	color_feedback = color;

	gl_Position = vec4(float(gl_VertexID) * 2.0 / num_planets - 1.0, 0.0, 0.0, 1.0);
}
