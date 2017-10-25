#version 440
#extension GL_ARB_separate_shader_objects : enable

layout(vertices = 1) out;

void main() {
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	gl_TessLevelInner[0] = 2.0;
	gl_TessLevelInner[1] = 2.0;
	gl_TessLevelOuter[0] = 2.0;
	gl_TessLevelOuter[1] = 2.0;
	gl_TessLevelOuter[2] = 2.0;
	gl_TessLevelOuter[3] = 2.0;
}