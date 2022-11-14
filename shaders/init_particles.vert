#version 300 es

out vec2 init_speed;
out vec2 init_current_pos;
out vec4 init_color;

uniform int width;
uniform int height;

void main()
{
	float x = (float(gl_VertexID % width) + 0.5) * (2.0 / float(width)) - 1.0;
	float y = (float(gl_VertexID / width) + 0.5) * (2.0 / float(height)) - 1.0;

	float r = 0.2 * float(gl_VertexID % 5) + 0.1;
	float g = 0.2 * float((gl_VertexID / 5) % 5) + 0.1;
	float b = 0.04 * float(gl_VertexID % 25) + 0.02;

	init_speed = vec2(0.0);
	init_current_pos = vec2(x, y);
	init_color = vec4(r, g, b, 1.0);
}
