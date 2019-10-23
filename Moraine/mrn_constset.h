#pragma once

#include "mrn_core.h"
#include "mrn_shader.h"

namespace moraine
{
    enum ConstantResourceType
    {
        CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER,
        CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER_DYNAMIC,
        CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER,
        CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER_DYNAMIC,
        CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER
    };

    class ConstantResource
    {
    public:
        ConstantResource(ConstantResourceType type) :
            m_type(type)
        { }

        virtual ~ConstantResource() = default;

        ConstantResourceType m_type;
    };

    class ConstantSet_T
    {
    public:
        virtual ~ConstantSet_T() = default;
    };

    typedef std::shared_ptr<ConstantSet_T> ConstantSet;

    MRN_API ConstantSet createConstantSet(Shader shader, uint32_t set, std::vector<std::pair<ConstantResource, uint32_t>>& resources);
}