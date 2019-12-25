#include "mrn_core.h"
#include "mrn_texture_vk.h"
#include "mrn_buffer_vk.h"

#include <stb_image.h>

#ifdef assert
#undef assert
#endif

moraine::Texture_IVulkan::Texture_IVulkan(GraphicsContext context, Stringr imagePath, uint32_t textureFlags) :
    Texture_T(0, 0),
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_format(VK_FORMAT_R8G8B8A8_UNORM),
    m_textureFlags(textureFlags)
{
    int width, height, channelCount;
    void* data = stbi_load(imagePath.mbstr(), &width, &height, &channelCount, STBI_rgb_alpha);

    assert(m_context->getLogfile(), !!data, sprintf(L"Loading texture \"%s\" failed", imagePath), MRN_DEBUG_INFO);

    m_width = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    void* stagingPointer;

    size_t stagingBufferSize = width * height * STBI_rgb_alpha * sizeof(uint8_t);

    m_context->createVulkanBuffer(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                                  &stagingBuffer, &stagingAllocation, &stagingPointer);

    memcpy_s(stagingPointer, stagingBufferSize, data, stagingBufferSize);

    stbi_image_free(data);

    VkExtent3D imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    m_context->createVulkanImage(VK_FORMAT_R8G8B8A8_UNORM, imageExtent.width, imageExtent.height,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                 &m_image, &m_allocation, &m_imageView);

    

    m_context->dispatchTask(m_context->m_transferQueue, [this, imageExtent, stagingBuffer](VkCommandBuffer buffer)
    {
        VkImageMemoryBarrier barrier;
        barrier.sType                               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext                               = nullptr;
        barrier.srcAccessMask                       = 0;
        barrier.dstAccessMask                       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout                           = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex                 = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex                 = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                               = m_image;
        barrier.subresourceRange.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel       = 0;
        barrier.subresourceRange.levelCount         = 1;
        barrier.subresourceRange.baseArrayLayer     = 0;
        barrier.subresourceRange.layerCount         = 1;

        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        VkBufferImageCopy copy;
        copy.bufferOffset                           = 0;
        copy.bufferRowLength                        = 0; // tight packed data
        copy.bufferImageHeight                      = 0;
        copy.imageSubresource.aspectMask            = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel              = 0;
        copy.imageSubresource.baseArrayLayer        = 0;
        copy.imageSubresource.layerCount            = 1;
        copy.imageOffset                            = { 0, 0, 0 };
        copy.imageExtent                            = imageExtent;

        vkCmdCopyBufferToImage(buffer,
                               stagingBuffer, m_image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &copy);

        barrier.srcAccessMask                       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                       = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout                           = barrier.newLayout;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    });

    vmaDestroyBuffer(m_context->m_allocator, stagingBuffer, stagingAllocation);

    

    constructVulkanSampler();
}

moraine::Texture_IVulkan::Texture_IVulkan(GraphicsContext context, ImageColorChannels channels, uint32_t width, uint32_t height, uint32_t textureFlags) :
    Texture_T(width, height),
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_textureFlags(textureFlags)
{
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    switch (channels)
    {
    case IMAGE_COLOR_CHANNELS_BW:       format = VK_FORMAT_R8_UNORM;     break;
    case IMAGE_COLOR_CHANNELS_BW_ALPHA: format = VK_FORMAT_R8G8_UNORM;   break;
    case IMAGE_COLOR_CHANNELS_RGB:      format = VK_FORMAT_R8G8B8_UNORM; break;
    }

    m_format = format;

    m_context->createVulkanImage(format, width, height,
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                 &m_image, &m_allocation, &m_imageView);

    m_context->dispatchTask(m_context->m_transferQueue, [this](VkCommandBuffer buffer)
    {
        VkImageMemoryBarrier barrier;
        barrier.sType                               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext                               = nullptr;
        barrier.srcAccessMask                       = 0;
        barrier.dstAccessMask                       = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout                           = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex                 = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex                 = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                               = m_image;
        barrier.subresourceRange.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel       = 0;
        barrier.subresourceRange.levelCount         = 1;
        barrier.subresourceRange.baseArrayLayer     = 0;
        barrier.subresourceRange.layerCount         = 1;

        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    });

    constructVulkanSampler();
}

moraine::Texture_IVulkan::~Texture_IVulkan()
{
    vkDestroySampler(m_context->m_device, m_sampler, nullptr);
    vkDestroyImageView(m_context->m_device, m_imageView, nullptr);
    vmaDestroyImage(m_context->m_allocator, m_image, m_allocation);
}

void moraine::Texture_IVulkan::resize(uint32_t width, uint32_t height)
{
    VkImage newImage;
    VkImageView newImageView;
    VmaAllocation newImageAllocation;

    m_context->createVulkanImage(m_format, width, height,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                 &newImage, &newImageAllocation, &newImageView);

    uint32_t copyWidth = min(m_width, width);
    uint32_t copyHeight = min(m_height, height);

    m_context->dispatchTask(m_context->m_transferQueue, [this, newImage, copyWidth, copyHeight](VkCommandBuffer buffer)
    {
        VkImageMemoryBarrier barrier;
        barrier.sType                               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext                               = nullptr;
        barrier.srcAccessMask                       = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask                       = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout                           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex                 = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex                 = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                               = m_image;
        barrier.subresourceRange.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel       = 0;
        barrier.subresourceRange.levelCount         = 1;
        barrier.subresourceRange.baseArrayLayer     = 0;
        barrier.subresourceRange.layerCount         = 1;

        // Prepare old image for transfer read
        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        barrier.image                               = newImage;
        barrier.srcAccessMask                       = 0;
        barrier.dstAccessMask                       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout                           = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        // Prepare new image for transfer write
        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        VkImageCopy copy;
        copy.srcSubresource.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.srcSubresource.baseArrayLayer          = 0;
        copy.srcSubresource.layerCount              = 1;
        copy.srcSubresource.mipLevel                = 0;
        copy.srcOffset                              = { 0, 0, 0 };
        copy.dstSubresource.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.dstSubresource.baseArrayLayer          = 0;
        copy.dstSubresource.layerCount              = 1;
        copy.dstSubresource.mipLevel                = 0;
        copy.dstOffset                              = { 0, 0, 0 };
        copy.extent                                 = { copyWidth, copyHeight, 1 };

        // Copy image
        vkCmdCopyImage(buffer,
                       m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       newImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &copy);

        barrier.image                               = m_image;
        barrier.srcAccessMask                       = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask                       = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout                           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Transfer old image to shader read (for non updated command buffers)
        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        barrier.image                               = newImage;
        barrier.srcAccessMask                       = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                       = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout                           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Transfer new image to shader read
        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    });

    std::swap(m_allocation, newImageAllocation);
    std::swap(m_image, newImage);
    std::swap(m_imageView, newImageView);

    m_width = width;
    m_height = height;

    m_context->addAsyncTask(
        [this](uint32_t frameIndex)
        {
        for (auto& a : m_constantSetBindings)
            a.first->updateDescriptorSet(std::make_pair(this, a.second), frameIndex);
        },
        [this, newImage, newImageView, newImageAllocation]()
        {
            vkDestroyImageView(m_context->m_device, newImageView, nullptr);
            vmaDestroyImage(m_context->m_allocator, newImage, newImageAllocation);
        });
}

void moraine::Texture_IVulkan::copyBufferRegionsToTexture(std::vector<CopySubImage>& regions, std::function<void()> operationOnComplete)
{
    std::vector<VkBufferImageCopy> vulkanRegions(regions.size());

    for(size_t i = 0; i < regions.size(); ++i)
        vulkanRegions[i] =
        {
            static_cast<VkDeviceSize>(static_cast<uint8_t*>(regions[i].stagingStackLocation) - 
                static_cast<uint8_t*>(std::static_pointer_cast<StagingStack_IVulkan>(m_context->m_stagingStack)->getStart())),  // bufferOffset
            0,                                                                                                                  // bufferRowLength
            0,                                                                                                                  // bufferImageHeight
            {                                                                                                                   // imageSubresource
                VK_IMAGE_ASPECT_COLOR_BIT,                                                                                      // aspectMask
                0,                                                                                                              // mipLevel
                0,                                                                                                              // baseArrayLayer
                1,                                                                                                              // layerCount
            },
            { static_cast<int32_t>(regions[i].imageLocation.x), static_cast<int32_t>(regions[i].imageLocation.y), 0 },          // imageOffset
            { regions[i].imageLocation.width, regions[i].imageLocation.height, 1 },                                             // imageExtent
        };

    m_context->dispatchTask(m_context->m_transferQueue, [vulkanRegions, this] (VkCommandBuffer buffer)
    {
        VkImageMemoryBarrier barrier;
        barrier.sType                                        = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext                                        = nullptr;
        barrier.srcQueueFamilyIndex                          = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex                          = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                                        = m_image;
        barrier.subresourceRange.aspectMask                  = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel                = 0;
        barrier.subresourceRange.levelCount                  = 1;
        barrier.subresourceRange.baseArrayLayer              = 0;
        barrier.subresourceRange.layerCount                  = 1;

        barrier.srcAccessMask                                = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask                                = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout                                    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout                                    = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        vkCmdCopyBufferToImage(buffer, std::static_pointer_cast<StagingStack_IVulkan>(m_context->m_stagingStack)->getBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(vulkanRegions.size()), vulkanRegions.data());

        barrier.srcAccessMask                                = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                                = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout                                    = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                                    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    });
}


void moraine::Texture_IVulkan::constructVulkanSampler()
{
    VkSamplerCreateInfo vsci;
    vsci.sType                                      = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    vsci.pNext                                      = nullptr;
    vsci.flags                                      = 0;
    vsci.magFilter                                  = VK_FILTER_LINEAR;
    vsci.minFilter                                  = VK_FILTER_LINEAR;
    vsci.mipmapMode                                 = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    vsci.addressModeU                               = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vsci.addressModeV                               = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vsci.addressModeW                               = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vsci.mipLodBias                                 = 0.0f;
    vsci.anisotropyEnable                           = VK_TRUE;
    vsci.maxAnisotropy                              = 16.0f;
    vsci.compareEnable                              = false;
    vsci.compareOp                                  = VK_COMPARE_OP_ALWAYS;
    vsci.minLod                                     = 0.0f;
    vsci.maxLod                                     = 0.0f;
    vsci.borderColor                                = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    vsci.unnormalizedCoordinates                    = m_textureFlags & UNNORMALIZED_UV_COORDINATES ? VK_TRUE : VK_FALSE;

    assert_vulkan(m_context->getLogfile(), vkCreateSampler(m_context->m_device, &vsci, nullptr, &m_sampler), L"vkCreateSampler() failed", MRN_DEBUG_INFO);
}