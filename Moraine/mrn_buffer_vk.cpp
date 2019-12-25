#include "mrn_core.h"
#include "mrn_buffer_vk.h"


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
        m_context->createVulkanBuffer(size, usage, VMA_MEMORY_USAGE_CPU_TO_GPU,
                                      &m_buffer, &m_allocation, keepMapped ? &m_data : nullptr);

        if (data != nullptr)
        {
            if (keepMapped)
                memcpy_s(m_data, size, data, size);
            else
            {
                void* mappedMemory;
                assert_vulkan(m_context->getLogfile(), vmaMapMemory(m_context->m_allocator, m_allocation, &mappedMemory), L"vmaMapMemory() failed", MRN_DEBUG_INFO);
                memcpy_s(mappedMemory, size, data, size);
                vmaUnmapMemory(m_context->m_allocator, m_allocation);
            }
        }
    }
    else
    {
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        void* stagingMemory;

        m_context->createVulkanBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                                      &stagingBuffer, &stagingAllocation, &stagingMemory);

        assert(m_context->getLogfile(), data != nullptr, L"Invalid API Usage: Data for VRAM buffers must be provided!", MRN_DEBUG_INFO);
        memcpy_s(stagingMemory, size, data, size);

        m_context->createVulkanBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                                      &m_buffer, &m_allocation, nullptr);

        m_context->dispatchTask(m_context->m_transferQueue, [size, this, stagingBuffer](VkCommandBuffer buffer)
        {
            VkBufferCopy copy;
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size      = size;

            vkCmdCopyBuffer(buffer, stagingBuffer, m_buffer, 1, &copy);
        });

        vmaDestroyBuffer(m_context->m_allocator, stagingBuffer, stagingAllocation);
    }
}

moraine::Buffer_IVulkan::~Buffer_IVulkan()
{
    vmaDestroyBuffer(m_context->m_allocator, m_buffer, m_allocation);
}

moraine::VertexBuffer_IVulkan::VertexBuffer_IVulkan(GraphicsContext context, size_t size, void* data, bool frequentUpdate, size_t reservedSize) :
    Buffer_IVulkan(context, max(size, reservedSize), data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, not frequentUpdate, frequentUpdate)
{
    m_usedSize = size;
    m_reservedSize = max(size, reservedSize);
}

moraine::VertexBuffer_IVulkan::~VertexBuffer_IVulkan()
{
}

void moraine::VertexBuffer_IVulkan::bind(VkCommandBuffer buffer, uint32_t binding, size_t offset)
{
    vkCmdBindVertexBuffers(buffer, binding, 1, &m_buffer, &offset);
}

void* moraine::VertexBuffer_IVulkan::data()
{
    assert(m_context->getLogfile(), m_data, L"Vertex Buffer not host visible!", MRN_DEBUG_INFO);
    return m_data;
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

moraine::ConstantArray_IVulkan::ConstantArray_IVulkan(GraphicsContext context, size_t elementSize, uint32_t initialElementCount, bool updateEveryFrame) :
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
    uint32_t index = static_cast<uint32_t>((reinterpret_cast<size_t>(m_head) - reinterpret_cast<size_t>(m_data)) / m_elementAlignedSize);

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

        assert_vulkan(m_context->getLogfile(), vmaCreateBuffer(m_context->m_allocator, &bufferInfo, &allocationInfo, &buffer, &allocation, &allocInfo), L"", MRN_DEBUG_INFO);

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




moraine::StagingStack_IVulkan::StagingStack_IVulkan(GraphicsContext context, size_t size)
    : Buffer_IVulkan(context, size, nullptr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, true),
    m_head(m_data),
    m_end(static_cast<uint8_t*>(m_data) + size)
{
}

moraine::StagingStack_IVulkan::~StagingStack_IVulkan()
{
}

moraine::StagingStackMarker moraine::StagingStack_IVulkan::createMarker()
{
    return m_head; // Return current position of head
}

void* moraine::StagingStack_IVulkan::alloc(size_t size, size_t alignment)
{
    void* head = m_head = getAlignedPointer(m_head, alignment); // Align head and copy it
    m_head = static_cast<uint8_t*>(m_head) + size; // Move head by size

    if (m_head >= m_end) // Stack overflow
        throw std::exception("Stack overflow");

    return head; // Return copied head
}

void moraine::StagingStack_IVulkan::free(StagingStackMarker m)
{
    if (m > m_data and m < m_end)
        m_head = m; // Set head back to marker
    else
        throw std::exception("Invalid StaginStackMarker!");
}
