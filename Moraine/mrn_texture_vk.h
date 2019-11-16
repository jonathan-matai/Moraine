#pragma once

#include "mrn_texture.h"
#include "mrn_gfxcontext_vk.h"

namespace moraine
{
    class Texture_IVulkan : public Texture_T
    {
        friend class ConstantSet_IVulkan;

    public:

        Texture_IVulkan(GraphicsContext context, Stringr imagePath);
        ~Texture_IVulkan() override;

    private:

        std::shared_ptr<GraphicsContext_IVulkan> m_context;

        VkImage         m_image;
        VmaAllocation   m_allocation;
        VkImageView     m_imageView;
        VkSampler       m_sampler;
    };
}