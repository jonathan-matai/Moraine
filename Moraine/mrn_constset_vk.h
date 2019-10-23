#pragma once

#include "mrn_constset.h"
#include "mrn_shader_vk.h"

namespace moraine
{
    class ConstantSet_IVulkan : public ConstantSet_T
    {
    public:

        ConstantSet_IVulkan(Shader context, uint32_t set, std::vector<std::pair<ConstantResource, uint32_t>>& resources);
        ~ConstantSet_IVulkan() override;

        VkDescriptorType getVulkanDescriptorType(ConstantResourceType type);

        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::shared_ptr<Shader_IVulkan> m_shader;
        std::shared_ptr<GraphicsContext_IVulkan> m_context;
    };
}