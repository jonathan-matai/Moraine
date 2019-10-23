#pragma once

#include "mrn_buffer.h"
#include "mrn_gfxcontext_vk.h"

namespace moraine
{
    class Buffer_IVulkan
    {
    protected:
        Buffer_IVulkan(GraphicsContext context, size_t size, void* data, VkBufferUsageFlags usage, bool useVram);
        virtual ~Buffer_IVulkan();

        VmaAllocation m_allocation;
        VkBuffer m_buffer;
        std::shared_ptr<GraphicsContext_IVulkan> m_context;
    };

    class VertexBuffer_IVulkan : public VertexBuffer_T, Buffer_IVulkan
    {
    public:

        VertexBuffer_IVulkan(GraphicsContext context, size_t size, void* data);
        ~VertexBuffer_IVulkan() override;

        inline VkBuffer getBuffer() const { return m_buffer; }
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
}