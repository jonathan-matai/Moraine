#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 out_texpoint;
layout(location = 1) out vec3 out_color;

layout(location = 0) in uvec4 texcoords;
layout(location = 1) in vec2 position;
layout(location = 2) in float size;
layout(location = 3) in uint color;

layout(binding = 1) uniform UBO
{
	vec2 stringPosition;
    vec2 padding;
	vec2 viewportSize;
} ubo;

vec2 positions[6] = vec2[](
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1),
    vec2(1, 1),
    vec2(0, 1),
    vec2(0, 0)
);

void main()
{
	vec2 pos = (ubo.stringPosition + position + vec2(float(texcoords.z), float(texcoords.w)) * positions[gl_VertexIndex] * size) / /*vec2(1600.0, 900.0)*/ubo.viewportSize * 2.0 - 1.0;
	gl_Position = vec4(pos, 0.0, 1.0);

	out_texpoint = texcoords.xy + positions[gl_VertexIndex] * texcoords.zw;

    out_color = vec3((color & 0x00ff0000) >> 16,
                          (color & 0x0000ff00) >> 8,
                          (color & 0x000000ff));
}