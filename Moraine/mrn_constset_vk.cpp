#include "mrn_constset_vk.h"

moraine::ConstantSet_IVulkan::ConstantSet_IVulkan(Shader shader, uint32_t set, std::vector<std::pair<ConstantResource, uint32_t>>& resources) :
    m_shader(std::static_pointer_cast<Shader_IVulkan>(shader)),
    m_context(m_shader->m_context)
{
    std::vector<VkDescriptorPoolSize> poolSizes;

    uint32_t nSwapchainImages = static_cast<uint32_t>(m_context->m_swapchainImages.size());

    for (const auto& a : resources)
    {
        VkDescriptorType type = getVulkanDescriptorType(a.first.m_type);

        auto element = std::find_if(poolSizes.begin(), poolSizes.end(), [type](const VkDescriptorPoolSize& poolSize) { return poolSize.type == type; });

        if (element == poolSizes.end())
            poolSizes.push_back({ type, nSwapchainImages });
        else
            element->descriptorCount += nSwapchainImages;
    }

    VkDescriptorPoolCreateInfo vdpci;
    vdpci.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vdpci.pNext                         = nullptr;
    vdpci.flags                         = 0;
    vdpci.maxSets                       = nSwapchainImages;
    vdpci.poolSizeCount                 = static_cast<uint32_t>(poolSizes.size());
    vdpci.pPoolSizes                    = poolSizes.data();

    assert_vulkan(m_context->m_logfile, vkCreateDescriptorPool(m_context->m_device, &vdpci, nullptr, &m_descriptorPool), L"vkCreateDescriptorPool() failed", MRN_DEBUG_INFO);

    std::vector<VkDescriptorSetLayout> layouts(nSwapchainImages, m_shader->m_descriptorLayouts[set]);

    m_descriptorSets.resize(nSwapchainImages);

    VkDescriptorSetAllocateInfo vdsai;
    vdsai.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vdsai.pNext                         = nullptr;
    vdsai.descriptorPool                = m_descriptorPool;
    vdsai.descriptorSetCount            = nSwapchainImages;
    vdsai.pSetLayouts                   = layouts.data();

    assert_vulkan(m_context->m_logfile, vkAllocateDescriptorSets(m_context->m_device, &vdsai, m_descriptorSets.data()), L"vkAllocateDescriptorSets() failed", MRN_DEBUG_INFO);

    // update descriptor sets
}

moraine::ConstantSet_IVulkan::~ConstantSet_IVulkan()
{
    vkDestroyDescriptorPool(m_context->m_device, m_descriptorPool, nullptr);
}

VkDescriptorType moraine::ConstantSet_IVulkan::getVulkanDescriptorType(ConstantResourceType type)
{
    switch (type)
    {
    case CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER:            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER_DYNAMIC:    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER:             return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER_DYNAMIC:     return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:     return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:                                                return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}
