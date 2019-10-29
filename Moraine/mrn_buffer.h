#pragma once

#include "mrn_core.h"
#include "mrn_gfxcontext.h"
#include "mrn_constset.h"

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

    class ConstantBuffer_T : public ConstantResource_T
    {
    public:

        ConstantBuffer_T() : 
            ConstantResource_T(CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER)
        { }

        virtual ~ConstantBuffer_T() = default;

        virtual void* data(uint32_t frameIndex) = 0;
    };

    typedef std::shared_ptr<ConstantBuffer_T> ConstantBuffer;

    MRN_API ConstantBuffer createConstantBuffer(GraphicsContext context, size_t size, bool updateEveryFrame);

    class ConstantArray_T : public ConstantResource_T
    {
    public:

        ConstantArray_T() :
            ConstantResource_T(CONSTANT_RESOURCE_TYPE_CONSTANT_ARRAY)
        { }

        virtual ~ConstantArray_T() = default;

        virtual uint32_t addElement() = 0;
        virtual void removeElement(uint32_t element) = 0;

        virtual void* data(uint32_t frameIndex, uint32_t elementIndex) = 0;
    };

    typedef std::shared_ptr<ConstantArray_T> ConstantArray;

    MRN_API ConstantArray createConstantArray(GraphicsContext context, size_t elementSize, size_t initialElementCount, bool updateEveryFrame);
}