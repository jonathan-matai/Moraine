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

moraine::ConstantBuffer moraine::createConstantBuffer(GraphicsContext context, size_t size, bool updateEveryFrame)
{
    return std::make_shared<ConstantBuffer_IVulkan>(context, size, updateEveryFrame);
}

moraine::ConstantArray moraine::createConstantArray(GraphicsContext context, size_t elementSize, size_t initialElementCount, bool updateEveryFrame)
{
    return std::make_shared<ConstantArray_IVulkan>(context, elementSize, initialElementCount, updateEveryFrame);
}
