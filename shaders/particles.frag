#version 300 es

in mediump vec3 frag_color;

out mediump vec4 out_color;

void main()
{
	out_color = vec4(frag_color, 1.0);
}
