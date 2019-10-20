#include "mrn_shader.h"
#include "mrn_shader_vk.h"

moraine::Shader moraine::createShader(Stringr shader, GraphicsContext context)
{
    return std::make_shared<Shader_IVulkan>(shader, context);
}
