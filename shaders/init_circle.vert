#version 300 es

out vec2 position;

uniform int num_sides;

const float PI = 3.141592653589793;

void main()
{
	if (gl_VertexID == 0) {
		position = vec2(0.0, 0.0);
	} else {
		float x = cos(2.0 * PI * float(gl_VertexID) / float(num_sides));
		float y = sin(2.0 * PI * float(gl_VertexID) / float(num_sides));
		position = vec2(x, y);
	}
}
