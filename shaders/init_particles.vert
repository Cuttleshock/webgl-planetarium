#version 300 es

out vec2 init_speed;
out vec4 init_color;

void main()
{
	float r = 0.2 * float(gl_VertexID % 5) + 0.1;
	float g = 0.2 * float((gl_VertexID / 5) % 5) + 0.1;
	float b = 0.04 * float(gl_VertexID % 25) + 0.02;

	init_speed = vec2(0.0);
	init_color = vec4(r, g, b, 1.0);
}
