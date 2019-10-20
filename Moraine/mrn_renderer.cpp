#include "mrn_renderer.h"
#include "mrn_renderer_vk.h"

moraine::Renderer moraine::createRenderer(GraphicsContext context)
{
    return std::make_shared<Renderer_IVulkan>(context);
}
