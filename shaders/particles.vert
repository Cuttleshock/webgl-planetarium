#version 300 es

in vec2 vert_displacement;
in vec4 color;

uniform sampler2D positions;
uniform float planet_r; // Radius of each planet relative to screen
uniform vec2 camera;

out vec4 frag_color;

void main()
{
	vec2 current_pos = texelFetch(positions, ivec2(gl_InstanceID, 0), 0).xy;
	gl_Position = vec4(current_pos + planet_r * vert_displacement - camera, 0.0, 1.0);
	frag_color = color;
}
