#version 300 es

out vec3 frag_color;

out vec2 init_home_pos;
out vec2 init_speed;
out vec2 init_current_pos;

uniform int width;
uniform int height;

void main()
{
	float x = (float(gl_VertexID % width) + 0.5) * (2.0 / float(width)) - 1.0;
	float y = (float(gl_VertexID / width) + 0.5) * (2.0 / float(height)) - 1.0;

	init_home_pos = vec2(x, y);
	init_speed = vec2(0.0);
	init_current_pos = vec2(x, y);
}
