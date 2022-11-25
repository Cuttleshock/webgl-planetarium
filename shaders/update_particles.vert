#version 300 es

in vec2 speed;
in vec4 color;

out vec2 speed_feedback;
out vec4 color_feedback;

uniform float num_planets;
uniform sampler2D positions;
uniform sampler2D attractions;

const float time_step = 0.0001; // For applying accel to speed.
// Speed is then treated as time-normalised.
const float damping = 0.995; // Prevent the system from accumulating energy

void main()
{
	vec2 current_pos = texelFetch(positions, ivec2(gl_VertexID, 0), 0).xy;
	vec2 accel = texelFetch(attractions, ivec2(0, gl_VertexID), 0).xy;

	speed_feedback = (speed + accel * time_step) * damping;
	color_feedback = color;

	gl_Position = vec4(float(gl_VertexID) * 2.0 / num_planets - 1.0, 0.0, 0.0, 1.0);
}
