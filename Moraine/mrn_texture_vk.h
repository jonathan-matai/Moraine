#pragma once

#include "mrn_texture.h"
#include "mrn_gfxcontext_vk.h"

namespace moraine
{
    class Texture_IVulkan : public Texture_T
    {
        friend class ConstantSet_IVulkan;

    public:

        Texture_IVulkan(GraphicsContext context, Stringr imagePath, uint32_t textureFlags);
        Texture_IVulkan(GraphicsContext context, ImageColorChannels channels, uint32_t width, uint32_t height, uint32_t textureFlags);
        ~Texture_IVulkan() override;

        void resize(uint32_t width, uint32_t height) override;
        void copyBufferRegionsToTexture(std::vector<CopySubImage>& regions, std::function<void()> operationOnComplete) override;

    protected:

        std::shared_ptr<GraphicsContext_IVulkan> m_context;

        void constructVulkanSampler();

        VkImage         m_image;
        VmaAllocation   m_allocation;
        VkImageView     m_imageView;
        VkSampler       m_sampler;
        VkFormat        m_format;
        uint32_t        m_textureFlags;

        std::vector<std::pair<ConstantSet_IVulkan*, uint32_t>> m_constantSetBindings;
    };
}