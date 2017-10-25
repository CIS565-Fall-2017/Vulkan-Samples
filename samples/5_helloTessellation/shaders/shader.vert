#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform ModelUBO {
	mat4 modelMatrix;
};

layout (location = 0) in vec3 position;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = modelMatrix * vec4(position, 1.0);
}