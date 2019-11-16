#include "mrn_texture_vk.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef assert
#undef assert
#endif

moraine::Texture_IVulkan::Texture_IVulkan(GraphicsContext context, Stringr imagePath) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context))
{
    int width, height, channelCount;
    void* data = stbi_load(imagePath.mbstr(), &width, &height, &channelCount, STBI_rgb_alpha);

    assert(m_context->m_logfile, !!data, sprintf(L"Loading texture \"%s\" failed", imagePath), MRN_DEBUG_INFO);

    VkFormat format;

    VkBufferCreateInfo vbci;
    vbci.sType                          = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vbci.pNext                          = nullptr;
    vbci.flags                          = 0;
    vbci.size                           = width * height * STBI_rgb_alpha * sizeof(uint8_t);
    vbci.usage                          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vbci.sharingMode                    = VK_SHARING_MODE_EXCLUSIVE;
    vbci.queueFamilyIndexCount          = 0;
    vbci.pQueueFamilyIndices            = nullptr;

    VmaAllocationCreateInfo vaci = { };
    vaci.usage                          = VMA_MEMORY_USAGE_CPU_ONLY;
    vaci.flags                          = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingInfo;

    assert_vulkan(m_context->m_logfile, vmaCreateBuffer(m_context->m_allocator, &vbci, &vaci, &stagingBuffer, &stagingAllocation, &stagingInfo), L"vmaCreateBuffer() failed", MRN_DEBUG_INFO);
    memcpy_s(stagingInfo.pMappedData, vbci.size, data, vbci.size);

    stbi_image_free(data);

    VkImageCreateInfo vici;
    vici.sType                                      = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vici.pNext                                      = nullptr;
    vici.flags                                      = 0;
    vici.imageType                                  = VK_IMAGE_TYPE_2D;
    vici.format                                     = VK_FORMAT_R8G8B8A8_UNORM;
    vici.extent                                     = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    vici.mipLevels                                  = 1;
    vici.arrayLayers                                = 1;
    vici.samples                                    = VK_SAMPLE_COUNT_1_BIT;
    vici.tiling                                     = VK_IMAGE_TILING_OPTIMAL;
    vici.usage                                      = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vici.sharingMode                                = VK_SHARING_MODE_EXCLUSIVE;
    vici.queueFamilyIndexCount                      = 0;
    vici.pQueueFamilyIndices                        = nullptr;
    vici.initialLayout                              = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo viaco = { };            
    viaco.usage                                     = VMA_MEMORY_USAGE_GPU_ONLY;

    assert_vulkan(m_context->m_logfile, vmaCreateImage(m_context->m_allocator, &vici, &viaco, &m_image, &m_allocation, nullptr), L"vmaCreateBuffer() failed", MRN_DEBUG_INFO);

    m_context->dispatchTask(m_context->m_transferQueue, [this, vici, stagingBuffer](VkCommandBuffer buffer)
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
        copy.imageExtent                            = vici.extent;

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

    VkImageViewCreateInfo vivci;
    vivci.sType                                     = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vivci.pNext                                     = nullptr;
    vivci.flags                                     = 0;
    vivci.image                                     = m_image;
    vivci.viewType                                  = VK_IMAGE_VIEW_TYPE_2D;
    vivci.format                                    = vici.format;
    vivci.components                                = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    vivci.subresourceRange.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
    vivci.subresourceRange.baseMipLevel             = 0;
    vivci.subresourceRange.levelCount               = 1;
    vivci.subresourceRange.baseArrayLayer           = 0;
    vivci.subresourceRange.layerCount               = 1;

    assert_vulkan(m_context->m_logfile, vkCreateImageView(m_context->m_device, &vivci, nullptr, &m_imageView), L"vkCreateImageView() failed", MRN_DEBUG_INFO);

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
    vsci.unnormalizedCoordinates                    = VK_FALSE;

    assert_vulkan(m_context->m_logfile, vkCreateSampler(m_context->m_device, &vsci, nullptr, &m_sampler), L"vkCreateSampler() failed", MRN_DEBUG_INFO);
}

moraine::Texture_IVulkan::~Texture_IVulkan()
{
    vkDestroySampler(m_context->m_device, m_sampler, nullptr);
    vkDestroyImageView(m_context->m_device, m_imageView, nullptr);
    vmaDestroyImage(m_context->m_allocator, m_image, m_allocation);
}
