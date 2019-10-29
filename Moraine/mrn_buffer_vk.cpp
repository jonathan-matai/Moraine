#include "mrn_buffer_vk.h"

#ifdef assert
#undef assert
#endif

moraine::Buffer_IVulkan::Buffer_IVulkan(GraphicsContext context, size_t size, void* data, VkBufferUsageFlags usage, bool useVram, bool keepMapped) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_data(nullptr)
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

    if (not useVram)
    {
        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage                    = VMA_MEMORY_USAGE_CPU_TO_GPU;

        if (keepMapped)
        {
            allocationInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocationInfo allocInfo;
            assert_vulkan(m_context->m_logfile, vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo, &m_buffer, &m_allocation, &allocInfo), L"vmaCreateBuffer() failed", MRN_DEBUG_INFO);

            m_data = allocInfo.pMappedData;

            if(data != nullptr)
                memcpy_s(m_data, size, data, size);
        }
        else
        {
            assert_vulkan(m_context->m_logfile, vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo, &m_buffer, &m_allocation, nullptr), L"vmaCreateBuffer failed", MRN_DEBUG_INFO);

            if (data != nullptr)
            {
                void* mappedMemory;
                assert_vulkan(m_context->m_logfile, vmaMapMemory(m_context->m_allocator, m_allocation, &mappedMemory), L"vmaMapMemory() failed", MRN_DEBUG_INFO);
                memcpy_s(mappedMemory, size, data, size);
                vmaUnmapMemory(m_context->m_allocator, m_allocation);
            }
        }
    }
    else
    {
        bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

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

        bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo2, &m_buffer, &m_allocation, nullptr);

        VkBuffer dstBuffer = m_buffer;

        m_context->dispatchTask(m_context->m_transferQueue, [size, dstBuffer, stagingBuffer](VkCommandBuffer buffer)
        {
            VkBufferCopy copy;
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size      = size;

            vkCmdCopyBuffer(buffer, stagingBuffer, dstBuffer, 1, &copy);
        });

        vmaDestroyBuffer(m_context->m_allocator, stagingBuffer, stagingAllocation);
    }
}

moraine::Buffer_IVulkan::~Buffer_IVulkan()
{
    vmaDestroyBuffer(m_context->m_allocator, m_buffer, m_allocation);
}

size_t moraine::Buffer_IVulkan::getAlignedSize(size_t rawSize, size_t alignment)
{
    return alignment > 0 ? (rawSize + alignment - 1) & ~(alignment - 1) : rawSize;
}

moraine::VertexBuffer_IVulkan::VertexBuffer_IVulkan(GraphicsContext context, size_t size, void* data) :
    Buffer_IVulkan(context, size, data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true, false)
{
}

moraine::VertexBuffer_IVulkan::~VertexBuffer_IVulkan()
{
}

void moraine::VertexBuffer_IVulkan::bind(VkCommandBuffer buffer, uint32_t binding, size_t offset)
{
    vkCmdBindVertexBuffers(buffer, binding, 1, &m_buffer, &offset);
}

moraine::IndexBuffer_IVulkan::IndexBuffer_IVulkan(GraphicsContext context, size_t indexCount, uint32_t* indexData) :
    Buffer_IVulkan(context, indexCount * sizeof(uint32_t), indexData, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true, false),
    m_32bitIndicies(true)
{
}

moraine::IndexBuffer_IVulkan::IndexBuffer_IVulkan(GraphicsContext context, size_t indexCount, uint16_t* indexData) :
    Buffer_IVulkan(context, indexCount * sizeof(uint16_t), indexData, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true, false),
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

moraine::ConstantBuffer_IVulkan::ConstantBuffer_IVulkan(GraphicsContext context, size_t size, bool updateEveryFrame) :
    Buffer_IVulkan(context, 
                   updateEveryFrame ? 
                       getAlignedSize(size, std::static_pointer_cast<GraphicsContext_IVulkan>(context)->m_physicalDevice.deviceProperties.limits.minUniformBufferOffsetAlignment) * 
                           std::static_pointer_cast<GraphicsContext_IVulkan>(context)->m_swapchainImages.size() :
                       size, 
                   nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false, true),
    m_elementSize(size),
    m_elementAlignedSize(updateEveryFrame ? getAlignedSize(size, std::static_pointer_cast<GraphicsContext_IVulkan>(context)->m_physicalDevice.deviceProperties.limits.minUniformBufferOffsetAlignment) : 0)
{
}

moraine::ConstantBuffer_IVulkan::~ConstantBuffer_IVulkan()
{
}

void* moraine::ConstantBuffer_IVulkan::data(uint32_t frameIndex)
{
    return static_cast<uint8_t*>(m_data) + frameIndex * m_elementAlignedSize;
}

moraine::ConstantArray_IVulkan::ConstantArray_IVulkan(GraphicsContext context, size_t elementSize, size_t initialElementCount, bool updateEveryFrame) :
    Buffer_IVulkan(context,
                   getAlignedSize(elementSize, std::static_pointer_cast<GraphicsContext_IVulkan>(context)->m_physicalDevice.deviceProperties.limits.minUniformBufferOffsetAlignment) *
                       initialElementCount *
                       (updateEveryFrame ? std::static_pointer_cast<GraphicsContext_IVulkan>(context)->m_swapchainImages.size() : 1),
                   nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, true),
    m_reservedElementCount(initialElementCount),
    m_elementAlignedSize(getAlignedSize(elementSize, std::static_pointer_cast<GraphicsContext_IVulkan>(context)->m_physicalDevice.deviceProperties.limits.minUniformBufferOffsetAlignment)),
    m_perFrameData(updateEveryFrame),
    m_elementCount(0),
    m_head(m_data)
{
    for (uint32_t i = 0; i < m_reservedElementCount - 1; ++i)
        *reinterpret_cast<void**>(static_cast<uint8_t*>(m_data) + m_elementAlignedSize * i) = static_cast<uint8_t*>(m_data) + m_elementAlignedSize * (i + 1); // set i-th element to the address of the next element

    *reinterpret_cast<void**>(static_cast<uint8_t*>(m_data) + m_elementAlignedSize * (m_reservedElementCount - 1)) = nullptr; // set last element to nullptr
}

moraine::ConstantArray_IVulkan::~ConstantArray_IVulkan()
{
}

uint32_t moraine::ConstantArray_IVulkan::addElement()
{
    uint32_t index = (reinterpret_cast<size_t>(m_head) - reinterpret_cast<size_t>(m_data)) / m_elementAlignedSize;

    m_head = *reinterpret_cast<void**>(m_head);

    if (not m_head)
    {
        VkBufferCreateInfo bufferInfo;
        bufferInfo.sType                        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext                        = nullptr;
        bufferInfo.flags                        = 0;
        bufferInfo.size                         = m_elementAlignedSize * m_reservedElementCount * 2 * (m_perFrameData ? m_context->m_swapchainImages.size() : 1);
        bufferInfo.usage                        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode                  = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount        = 0;
        bufferInfo.pQueueFamilyIndices          = nullptr;

        VmaAllocationCreateInfo allocationInfo = { };
        allocationInfo.usage                    = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocationInfo.flags                    = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocInfo;
        VkBuffer buffer;
        VmaAllocation allocation;

        assert_vulkan(m_context->m_logfile, vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo, &buffer, &allocation, &allocInfo), L"", MRN_DEBUG_INFO);

        if (m_perFrameData)
        {
            for (size_t i = 0; i < m_context->m_swapchainImages.size(); ++i)
                memcpy_s(static_cast<uint8_t*>(allocInfo.pMappedData) + m_elementAlignedSize * m_reservedElementCount * 2 * i,
                         m_elementAlignedSize* m_reservedElementCount * 2,
                         static_cast<uint8_t*>(m_data) + m_elementAlignedSize * m_reservedElementCount * i,
                         m_elementAlignedSize* m_reservedElementCount);
        }
        else
        {
            memcpy_s(allocInfo.pMappedData, m_elementAlignedSize * m_reservedElementCount * 2, m_data, m_elementAlignedSize * m_reservedElementCount);
        }

        std::swap(m_buffer, buffer);
        std::swap(m_allocation, allocation);
        std::swap(m_data, allocInfo.pMappedData);

        m_reservedElementCount *= 2;

        for (uint32_t i = index; i < m_reservedElementCount - 1; ++i)
            *reinterpret_cast<void**>(static_cast<uint8_t*>(m_data) + m_elementAlignedSize * i) = static_cast<uint8_t*>(m_data) + m_elementAlignedSize * (i + 1); // set i-th element to the address of the next element

        *reinterpret_cast<void**>(static_cast<uint8_t*>(m_data) + m_elementAlignedSize * (m_reservedElementCount - 1)) = nullptr; // set last element to nullptr

        m_head = static_cast<uint8_t*>(m_data) + (index + 1) * m_elementAlignedSize;
        // static_cast<uint8_t*>(m_data) + elementIndex * m_elementAlignedSize

        m_context->addAsyncTask(
            [this](uint32_t frameIndex)
            {
                for (auto& a : m_constantSetBindings)
                    a.first->updateDescriptorSet(std::make_pair(this, a.second), frameIndex);
            },
            [this, buffer, allocation]()
            {
                vmaDestroyBuffer(m_context->m_allocator, buffer, allocation);
            });

        /*
        
        Create new buffer with twice the size

        if m_perFrameData
            for all frames
                copy to new buffer
        else
            copy to new buffer

        swap buffers

        update descriptor sets // wait until all are completed

        delete old buffer
        */

    }

    return index;
}

void moraine::ConstantArray_IVulkan::removeElement(uint32_t element)
{
    auto el = reinterpret_cast<void**>(static_cast<uint8_t*>(m_data) + m_elementAlignedSize * element);

    *el = m_head;
    m_head = el;
}

void* moraine::ConstantArray_IVulkan::data(uint32_t frameIndex, uint32_t elementIndex)
{
    if (m_perFrameData)
        return static_cast<uint8_t*>(m_data) +  m_elementAlignedSize * (frameIndex * m_reservedElementCount + elementIndex);
    else
        return static_cast<uint8_t*>(m_data) + elementIndex * m_elementAlignedSize;
}