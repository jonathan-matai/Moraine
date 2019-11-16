#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 oTexpoint;
layout(location = 1) out vec3 oColor;

layout(location = 0) in vec2 iPos;
layout(location = 1) in vec2 iTex;

layout(binding = 1) uniform ConstantBuffer
{
    vec2 pos;
    vec3 color;
    vec4 angle;
} cb;

void main()
{
    float angle = cb.angle.x;

    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

	gl_Position = vec4(vec2(0.5, 1.0) * (rotation * iPos) + cb.pos, 0.0, 1.0);
	oTexpoint = iTex;
	oColor = cb.color;
}