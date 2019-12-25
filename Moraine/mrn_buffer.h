#pragma once

#include "mrn_constset.h"

namespace moraine
{
    MRN_DECLARE_HANDLE(GraphicsContext)

    typedef void* StagingStackMarker;

    class StagingStack_T
    {
    public:

        virtual ~StagingStack_T() = default;

        virtual StagingStackMarker createMarker() = 0;
        virtual void* alloc(size_t size, size_t alignment = alignof(void*)) = 0;
        virtual void free(StagingStackMarker m = 0) = 0;
    };

    typedef std::shared_ptr<StagingStack_T> StagingStack;

    MRN_API StagingStack createStagingStack(GraphicsContext context, size_t sizeInBytes);

    class VertexBuffer_T
    {
    public:

        virtual ~VertexBuffer_T() = default;

        size_t size() const { return m_usedSize; }

        virtual void* data() = 0;

    protected:

        size_t m_usedSize;
        size_t m_reservedSize;
    };

    typedef std::shared_ptr<VertexBuffer_T> VertexBuffer;

    MRN_API VertexBuffer createVertexBuffer(GraphicsContext context, size_t size, void* data, bool frequentUpdate, size_t reservedSize = 0);

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

    MRN_API ConstantArray createConstantArray(GraphicsContext context, size_t elementSize, uint32_t initialElementCount, bool updateEveryFrame);
}