#version 440
#extension GL_ARB_separate_shader_objects : enable

layout(quads, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform CameraUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
};

void main() {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	gl_Position = projectionMatrix * viewMatrix * (gl_in[0].gl_Position + vec4(1.0 - u, v, 0.0, 0.0));
}