#pragma once

namespace moraine
{
    MRN_DECLARE_HANDLE(Shader)

    enum ConstantResourceType
    {
        CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER,
        CONSTANT_RESOURCE_TYPE_CONSTANT_ARRAY,
        CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER,
        CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER_DYNAMIC,
        CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER
    };

    class ConstantResource_T
    {
    public:
        ConstantResource_T(ConstantResourceType type) :
            m_type(type)
        { }

        virtual ~ConstantResource_T() = default;

        ConstantResourceType m_type;
    };

    typedef std::shared_ptr<ConstantResource_T> ConstantResource;

    class ConstantSet_T
    {
    public:
        virtual ~ConstantSet_T() = default;
    };

    typedef std::shared_ptr<ConstantSet_T> ConstantSet;

    MRN_API ConstantSet createConstantSet(Shader shader, uint32_t set, std::initializer_list<std::pair<ConstantResource, uint32_t>> resources);
}