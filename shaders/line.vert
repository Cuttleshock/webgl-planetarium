#version 300 es

// A line for glDrawArrays(GL_LINES, 0, 2)

void main()
{
	gl_Position = vec4(-1.0 + 2.0 * float(gl_VertexID), 0.0, 0.0, 1.0);
}
