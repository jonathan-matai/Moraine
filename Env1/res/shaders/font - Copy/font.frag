#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 texcoord;

layout(binding = 0) uniform sampler2D smp;

layout(location = 0) out vec4 outColor;

void main() {
    float color = texture(smp, texcoord).r;

    outColor = vec4(1.0, 1.0, 1.0, color);
}