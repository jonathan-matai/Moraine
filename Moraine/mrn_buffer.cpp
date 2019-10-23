#include "mrn_buffer.h"
#include "mrn_buffer_vk.h"

moraine::VertexBuffer moraine::createVertexBuffer(GraphicsContext context, size_t size, void* data)
{
    return std::make_shared<VertexBuffer_IVulkan>(context, size, data);
}

moraine::IndexBuffer moraine::createIndexBuffer(GraphicsContext context, size_t indexCount, uint32_t* indexData)
{
    return std::make_shared<IndexBuffer_IVulkan>(context, indexCount, indexData);
}

moraine::IndexBuffer moraine::createIndexBuffer(GraphicsContext context, size_t indexCount, uint16_t* indexData)
{
    return std::make_shared<IndexBuffer_IVulkan>(context, indexCount, indexData);
}
