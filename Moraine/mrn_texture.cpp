#include "mrn_texture.h"
#include "mrn_texture_vk.h"

moraine::Texture moraine::createTexture(GraphicsContext context, Stringr imagePath)
{
    return std::make_shared<Texture_IVulkan>(context, imagePath);
}
