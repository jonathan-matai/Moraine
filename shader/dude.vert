#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform UB
{
    vec2 offset;
} ub;

void main() {
    //gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    //fragColor = colors[gl_VertexIndex];

    gl_Position = vec4(pos + ub.offset, 0.0, 1.0);
    fragColor = color;
}