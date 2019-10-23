#include "mrn_buffer_vk.h"

#ifdef assert
#undef assert
#endif

moraine::Buffer_IVulkan::Buffer_IVulkan(GraphicsContext context, size_t size, void* data, VkBufferUsageFlags usage, bool useVram) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context))
{
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType                        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext                        = nullptr;
    bufferInfo.flags                        = 0;
    bufferInfo.size                         = size;
    bufferInfo.usage                        = usage;
    bufferInfo.sharingMode                  = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount        = 0;
    bufferInfo.pQueueFamilyIndices          = nullptr;

    if (useVram)
    {
        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage                    = VMA_MEMORY_USAGE_CPU_TO_GPU;

        vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo, &m_buffer, &m_allocation, nullptr);

        if (data != nullptr)
        {
            void* mappedMemory;
            vmaMapMemory(m_context->m_allocator, m_allocation, &mappedMemory);
            memcpy_s(mappedMemory, size, data, size);
            vmaUnmapMemory(m_context->m_allocator, m_allocation);
        }
    }
    else
    {
        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage                    = VMA_MEMORY_USAGE_CPU_ONLY;
        allocationInfo.flags                    = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        VmaAllocationInfo stagingInfo;

        vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo, &stagingBuffer, &stagingAllocation, &stagingInfo);

        assert(m_context->m_logfile, data != nullptr, L"Invalid API Usage: Data for VRAM buffers must be provided!", MRN_DEBUG_INFO);
        memcpy_s(stagingInfo.pMappedData, size, data, size);

        VmaAllocationCreateInfo allocationInfo2 = { };
        allocationInfo2.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo2, &m_buffer, &m_allocation, nullptr);

        VkBuffer dstBuffer = m_buffer;

        m_context->dispatchTask(m_context->m_transferQueue, [size, dstBuffer, stagingBuffer](VkCommandBuffer buffer)
        {
            VkBufferCopy copy;
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size      = size;

            vkCmdCopyBuffer(buffer, dstBuffer, stagingBuffer, 1, &copy);
        });

        vmaDestroyBuffer(m_context->m_allocator, stagingBuffer, stagingAllocation);
    }
}

moraine::Buffer_IVulkan::~Buffer_IVulkan()
{
    vmaDestroyBuffer(m_context->m_allocator, m_buffer, m_allocation);
}

moraine::VertexBuffer_IVulkan::VertexBuffer_IVulkan(GraphicsContext context, size_t size, void* data) :
    Buffer_IVulkan(context, size, data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true)
{
}

moraine::VertexBuffer_IVulkan::~VertexBuffer_IVulkan()
{
}

moraine::IndexBuffer_IVulkan::IndexBuffer_IVulkan(GraphicsContext context, size_t indexCount, uint32_t* indexData) :
    Buffer_IVulkan(context, indexCount * sizeof(uint32_t), indexData, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true),
    m_32bitIndicies(true)
{
}

moraine::IndexBuffer_IVulkan::IndexBuffer_IVulkan(GraphicsContext context, size_t indexCount, uint16_t* indexData) :
    Buffer_IVulkan(context, indexCount * sizeof(uint16_t), indexData, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true),
    m_32bitIndicies(false)
{
}

moraine::IndexBuffer_IVulkan::~IndexBuffer_IVulkan()
{
}

void moraine::IndexBuffer_IVulkan::bind(VkCommandBuffer buffer)
{
    vkCmdBindIndexBuffer(buffer, m_buffer, 0, m_32bitIndicies ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}
