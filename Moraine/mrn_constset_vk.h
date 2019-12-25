#pragma once

#include "mrn_constset.h"

#include "mrn_shader_vk.h"
#include "mrn_buffer_vk.h"
#include "mrn_texture_vk.h"

#ifdef assert
#undef assert
#endif

namespace moraine
{
    class ConstantSet_IVulkan : public ConstantSet_T
    {
    public:

        ConstantSet_IVulkan(Shader context, uint32_t set, std::initializer_list<std::pair<ConstantResource, uint32_t>> resources);
        ~ConstantSet_IVulkan() override;

        void bind(VkCommandBuffer buffer, uint32_t frameIndex, std::initializer_list<uint32_t> arrayIndicies);
        void bind(VkCommandBuffer buffer, uint32_t frameIndex, const std::vector<uint32_t>& arrayIndicies);


        void updateDescriptorSets(std::initializer_list<std::pair<ConstantResource, uint32_t>> resources);
        void updateDescriptorSet(std::pair<ConstantResource_T*, uint32_t> resource, uint32_t frameIndex);

        VkDescriptorType getVulkanDescriptorType(ConstantResourceType type);

        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::shared_ptr<Shader_IVulkan> m_shader;
        uint32_t m_setIndex;

        // Stores the aligned element sizes of constant arrays and storage arrays
        std::vector<std::pair<size_t, uint32_t>> m_arrayAlignedElementSizes; // first: elementSize, second: binding
    };
}