#version 300 es

uniform sampler2D position_velocity;
uniform mediump float planet_r;

out mediump vec2 out_impulse;

const mediump float spring_k = 2000.0; // Displacement multiplier
const mediump float spring_b = 1000.0; // Velocity multiplier

void main()
{
	ivec2 discrete_coords = ivec2(gl_FragCoord);
	if (discrete_coords.x == discrete_coords.y) {
		out_impulse = vec2(0.0, 0.0);
		return;
	}

	mediump vec4 my_pv = texelFetch(position_velocity, ivec2(discrete_coords.x, 0), 0);
	mediump vec4 your_pv = texelFetch(position_velocity, ivec2(discrete_coords.y, 0), 0);

	mediump vec2 separation = (my_pv - your_pv).xy;
	mediump vec2 relative_v = (my_pv - your_pv).zw;

	if (length(separation) == 0.0) {
		out_impulse = vec2(1.0, 0.0);
	} else if (length(separation) < 2.0 * planet_r) {
		mediump vec2 spring_v = -normalize(separation) * dot(relative_v, normalize(separation));
		mediump vec2 spring_x = normalize(separation) * (2.0 * planet_r - length(separation));
		out_impulse = -spring_b * spring_v - spring_k * spring_x;
	} else {
		out_impulse = vec2(0.0, 0.0);
	}
}
