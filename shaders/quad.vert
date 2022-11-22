#version 300 es

// A quad for glDrawArrays(GL_TRIANGLES, 0, 6):
// 	1.0, 1.0,
// 	1.0, -1.0,
// 	-1.0, 1.0,
// 	-1.0, -1.0,
// 	-1.0, 1.0,
// 	1.0, -1.0,

void main()
{
	float x = 1.0 - 2.0 * float(((gl_VertexID + 1) / 3) % 2);
	float y = 1.0 - 2.0 * float(gl_VertexID % 2);
	gl_Position = vec4(x, y, 0.0, 1.0);
}
