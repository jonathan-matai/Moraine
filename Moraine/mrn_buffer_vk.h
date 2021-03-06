#pragma once

#include "mrn_buffer.h"
#include "mrn_constset_vk.h"

namespace moraine
{
    class Buffer_IVulkan
    {
    protected:

        Buffer_IVulkan(GraphicsContext context, size_t size, void* data, VkBufferUsageFlags usage, bool useVram, bool keepMapped);
        virtual ~Buffer_IVulkan();

        VmaAllocation m_allocation;
        VkBuffer m_buffer;
        void* m_data;
        std::shared_ptr<GraphicsContext_IVulkan> m_context;
    };

    class StagingStack_IVulkan : public StagingStack_T, Buffer_IVulkan
    {
    public:

        StagingStack_IVulkan(GraphicsContext context, size_t size);
        ~StagingStack_IVulkan() override;

        StagingStackMarker createMarker() override;
        void* alloc(size_t size, size_t alignment = alignof(void*)) override;
        void free(StagingStackMarker m = 0) override;

        void* getStart() const { return m_data; }
        VkBuffer getBuffer() const { return m_buffer; }

    private:

        void* m_head;
        void* m_end;
    };

    class VertexBuffer_IVulkan : public VertexBuffer_T, Buffer_IVulkan
    {
    public:

        VertexBuffer_IVulkan(GraphicsContext context, size_t size, void* data, bool frequentUpdate, size_t reservedSize);
        ~VertexBuffer_IVulkan() override;

        void bind(VkCommandBuffer buffer, uint32_t binding, size_t offset);

        void* data() override;
    };

    class IndexBuffer_IVulkan : public IndexBuffer_T, Buffer_IVulkan
    {
    public:

        IndexBuffer_IVulkan(GraphicsContext context, size_t indexCount, uint32_t* indexData);
        IndexBuffer_IVulkan(GraphicsContext context, size_t indexCount, uint16_t* indexData);
        ~IndexBuffer_IVulkan() override;

        void bind(VkCommandBuffer buffer);

    private:

        bool m_32bitIndicies;
    };

    class ConstantBuffer_IVulkan : public ConstantBuffer_T, Buffer_IVulkan
    {
        friend class ConstantSet_IVulkan;

    public:

        ConstantBuffer_IVulkan(GraphicsContext context, size_t size, bool updateEveryFrame);
        ~ConstantBuffer_IVulkan() override;

        void* data(uint32_t frameIndex) override;

        size_t m_elementAlignedSize;
        size_t m_elementSize;
    };

    class ConstantArray_IVulkan : public ConstantArray_T, Buffer_IVulkan
    {
        friend class ConstantSet_IVulkan;

    public:

        ConstantArray_IVulkan(GraphicsContext context, size_t elementSize, uint32_t initialElementCount, bool updateEveryFrame);
        ~ConstantArray_IVulkan() override;

        uint32_t addElement() override;
        void removeElement(uint32_t element) override;

        void* data(uint32_t frameIndex, uint32_t elementIndex) override;

        size_t m_elementAlignedSize;
        uint32_t m_elementCount;
        uint32_t m_reservedElementCount;
        bool m_perFrameData;
        std::vector<std::pair<ConstantSet_IVulkan*, uint32_t>> m_constantSetBindings;

        void* m_head;
    };
}