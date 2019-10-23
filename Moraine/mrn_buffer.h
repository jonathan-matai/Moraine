#pragma once

#include "mrn_core.h"
#include "mrn_gfxcontext.h"

namespace moraine
{
    class VertexBuffer_T
    {
    public:

        virtual ~VertexBuffer_T() = default;
    };

    typedef std::shared_ptr<VertexBuffer_T> VertexBuffer;

    MRN_API VertexBuffer createVertexBuffer(GraphicsContext context, size_t size, void* data);

    class IndexBuffer_T
    {
    public:

        virtual ~IndexBuffer_T() = default;
    };

    typedef std::shared_ptr<IndexBuffer_T> IndexBuffer;

    MRN_API IndexBuffer createIndexBuffer(GraphicsContext context, size_t indexCount, uint32_t* indexData);
    MRN_API IndexBuffer createIndexBuffer(GraphicsContext context, size_t indexCount, uint16_t* indexData);
}