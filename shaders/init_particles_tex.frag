#version 300 es

out mediump vec2 out_position;

uniform int width;
uniform int height;

void main()
{
	int i = int(gl_FragCoord.x);
	mediump float x = (float(i % width) + 0.5) * (2.0 / float(width)) - 1.0;
	mediump float y = (float(i / width) + 0.5) * (2.0 / float(height)) - 1.0;

	out_position = vec2(x, y);
}
