#version 300 es

out mediump vec4 out_position;

uniform sampler2D positions;
uniform sampler2D attractions;
uniform mediump float time_step; // For applying accel to speed

const mediump float time_scale = 0.01;

const mediump float damping = 0.995; // Prevent the system from accumulating energy

void main()
{
	mediump vec2 current_pos = texelFetch(positions, ivec2(int(gl_FragCoord.x), 0), 0).xy;
	mediump vec2 speed = texelFetch(positions, ivec2(int(gl_FragCoord.x), 0), 0).zw;
	mediump vec2 accel = texelFetch(attractions, ivec2(0, int(gl_FragCoord.x)), 0).xy;

	mediump vec2 new_speed = (speed + accel * time_step * time_scale) * damping;

	out_position = vec4(current_pos + new_speed, new_speed);
}
