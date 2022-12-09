#version 300 es

out vec2 speed_feedback;

uniform float num_planets;
uniform sampler2D positions;
uniform sampler2D attractions;
uniform float time_step; // For applying accel to speed

const float time_scale = 0.01;

const float damping = 0.995; // Prevent the system from accumulating energy

void main()
{
	vec2 current_pos = texelFetch(positions, ivec2(gl_VertexID, 0), 0).xy;
	vec2 speed = texelFetch(positions, ivec2(gl_VertexID, 0), 0).zw;
	vec2 accel = texelFetch(attractions, ivec2(0, gl_VertexID), 0).xy;

	speed_feedback = (speed + accel * time_step * time_scale) * damping;

	gl_Position = vec4((float(gl_VertexID) + 0.5) * 2.0 / num_planets - 1.0, 0.0, 0.0, 1.0);
}
