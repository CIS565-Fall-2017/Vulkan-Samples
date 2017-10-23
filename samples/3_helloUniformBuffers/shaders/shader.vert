#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraUBO {
	mat4 viewMatrix;
	mat4 projectionMatrix;
};

layout(set = 1, binding = 0) uniform ModelUBO {
	mat4 modelMatrix;
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    fragColor = color;
}