#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 iTexpoint;
layout(location = 1) in vec3 iColor;

layout(binding = 0) uniform sampler2D smp;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(smp, iTexpoint) * vec4(iColor, 1.0);
}