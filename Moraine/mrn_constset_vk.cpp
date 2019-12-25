#include "mrn_core.h"
#include "mrn_constset_vk.h"

moraine::ConstantSet_IVulkan::ConstantSet_IVulkan(Shader shader, uint32_t set, std::initializer_list<std::pair<ConstantResource, uint32_t>> resources) :
    m_shader(std::static_pointer_cast<Shader_IVulkan>(shader)),
    m_setIndex(set)
{
    uint32_t nSwapchainImages = static_cast<uint32_t>(m_shader->m_context->m_swapchainImages.size());

    VkDescriptorPoolCreateInfo vdpci;
    vdpci.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vdpci.pNext                         = nullptr;
    vdpci.flags                         = 0;
    vdpci.maxSets                       = nSwapchainImages;
    vdpci.poolSizeCount                 = static_cast<uint32_t>(m_shader->m_desriptorPoolSizes[set].size());
    vdpci.pPoolSizes                    = m_shader->m_desriptorPoolSizes[set].data();

    assert_vulkan(m_shader->m_logfile, vkCreateDescriptorPool(m_shader->m_context->m_device, &vdpci, nullptr, &m_descriptorPool), L"vkCreateDescriptorPool() failed", MRN_DEBUG_INFO);

    std::vector<VkDescriptorSetLayout> layouts(nSwapchainImages, m_shader->m_descriptorLayouts[set]);

    m_descriptorSets.resize(nSwapchainImages);

    VkDescriptorSetAllocateInfo vdsai;
    vdsai.sType                         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vdsai.pNext                         = nullptr;
    vdsai.descriptorPool                = m_descriptorPool;
    vdsai.descriptorSetCount            = nSwapchainImages;
    vdsai.pSetLayouts                   = layouts.data();

    assert_vulkan(m_shader->m_logfile, vkAllocateDescriptorSets(m_shader->m_context->m_device, &vdsai, m_descriptorSets.data()), L"vkAllocateDescriptorSets() failed", MRN_DEBUG_INFO);

    updateDescriptorSets(resources);
}

moraine::ConstantSet_IVulkan::~ConstantSet_IVulkan()
{
    vkDestroyDescriptorPool(m_shader->m_context->m_device, m_descriptorPool, nullptr);
}

void moraine::ConstantSet_IVulkan::bind(VkCommandBuffer buffer, uint32_t frameIndex, std::initializer_list<uint32_t> arrayIndicies)
{
    assert(m_shader->m_context->getLogfile(), arrayIndicies.size() == m_arrayAlignedElementSizes.size(), L"Wrong API Usage: Not all arrayIndicies provided!", MRN_DEBUG_INFO);

    std::vector<uint32_t> offsets(arrayIndicies.size());

    for (size_t i = 0; i < arrayIndicies.size(); ++i)
        offsets[i] = static_cast<uint32_t>(m_arrayAlignedElementSizes[i].first * arrayIndicies.begin()[i]);

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader->m_layout, m_setIndex, 1, &m_descriptorSets[frameIndex], static_cast<uint32_t>(offsets.size()), offsets.data());
}

void moraine::ConstantSet_IVulkan::bind(VkCommandBuffer buffer, uint32_t frameIndex, const std::vector<uint32_t>& arrayIndicies)
{
    assert(m_shader->m_logfile, arrayIndicies.size() == m_arrayAlignedElementSizes.size(), L"Wrong API Usage: Not all arrayIndicies provided!", MRN_DEBUG_INFO);

    std::vector<uint32_t> offsets(arrayIndicies.size());

    for (size_t i = 0; i < arrayIndicies.size(); ++i)
        offsets[i] = static_cast<uint32_t>(m_arrayAlignedElementSizes[i].first * arrayIndicies.begin()[i]);

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader->m_layout, m_setIndex, 1, &m_descriptorSets[frameIndex], static_cast<uint32_t>(offsets.size()), offsets.data());
}

void moraine::ConstantSet_IVulkan::updateDescriptorSets(std::initializer_list<std::pair<ConstantResource, uint32_t>> resources)
{
    std::vector<VkWriteDescriptorSet> writeSets;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;

    for (size_t i = 0; i < m_descriptorSets.size(); ++i)
    {
        for (auto& b : resources)
        {
            VkWriteDescriptorSet writeSet;
            writeSet.sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeSet.pNext                  = nullptr;
            writeSet.dstSet                 = m_descriptorSets[i];
            writeSet.dstBinding             = b.second;
            writeSet.dstArrayElement        = 0;
            writeSet.descriptorCount        = 1;
            writeSet.descriptorType         = getVulkanDescriptorType(b.first->m_type);
            writeSet.pImageInfo             = nullptr;
            writeSet.pBufferInfo            = nullptr;
            writeSet.pTexelBufferView       = nullptr;


            switch (b.first->m_type)
            {
            case CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER:
            {
                auto c = std::static_pointer_cast<ConstantBuffer_IVulkan>(b.first);
                bufferInfos.push_back({ c->m_buffer, c->m_elementAlignedSize * i, c->m_elementSize }); // if buffer doesn't have per frame data "m_elementAlignedSize" is 0, and no offset is applied
                writeSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(bufferInfos.size()); // write (vector index + 1) instead of pointer
                break;
            }

            case CONSTANT_RESOURCE_TYPE_CONSTANT_ARRAY:
            {
                auto c = std::static_pointer_cast<ConstantArray_IVulkan>(b.first);
                bufferInfos.push_back({ c->m_buffer, c->m_elementAlignedSize * c->m_reservedElementCount * (c->m_perFrameData ? i : 0), c->m_elementAlignedSize });
                writeSet.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(bufferInfos.size()); // write (vector index + 1) instead of pointer

                if (i == 0)
                {
                    m_arrayAlignedElementSizes.push_back(std::make_pair(c->m_elementAlignedSize, b.second));
                    c->m_constantSetBindings.push_back(std::pair<ConstantSet_IVulkan*, uint32_t>(this, b.second));
                }

                break;
            }

            case CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                auto c = std::static_pointer_cast<Texture_IVulkan>(b.first);
                imageInfos.push_back({ c->m_sampler, c->m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
                writeSet.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(imageInfos.size()); // write (vector index + 1) instead of pointer

                if (i == 0)
                {
                    c->m_constantSetBindings.push_back(std::pair<ConstantSet_IVulkan*, uint32_t>(this, b.second));
                }

                break;
            }

            default:
            {
                m_shader->m_logfile->print(RED, L"API ERROR: Unknown resource type!", MRN_DEBUG_INFO);
                throw std::exception();
            }
            }

            writeSets.push_back(writeSet);
        }
    }

    for (auto& a : writeSets)
        if (a.pBufferInfo != nullptr)
            a.pBufferInfo = &bufferInfos[reinterpret_cast<size_t>(a.pBufferInfo) - 1];
        else if (a.pImageInfo != nullptr)
            a.pImageInfo = &imageInfos[reinterpret_cast<size_t>(a.pImageInfo) - 1];

    vkUpdateDescriptorSets(m_shader->m_context->m_device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

void moraine::ConstantSet_IVulkan::updateDescriptorSet(std::pair<ConstantResource_T*, uint32_t> resource, uint32_t frameIndex)
{
    VkWriteDescriptorSet writeSet;
    writeSet.sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.pNext                  = nullptr;
    writeSet.dstSet                 = m_descriptorSets[frameIndex];
    writeSet.dstBinding             = resource.second;
    writeSet.dstArrayElement        = 0;
    writeSet.descriptorCount        = 1;
    writeSet.descriptorType         = getVulkanDescriptorType(resource.first->m_type);
    writeSet.pImageInfo             = nullptr;
    writeSet.pBufferInfo            = nullptr;
    writeSet.pTexelBufferView       = nullptr;


    switch (resource.first->m_type)
    {
    case CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER:
    {
        auto c = static_cast<ConstantBuffer_IVulkan*>(resource.first);

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer           = c->m_buffer;
        bufferInfo.offset           = c->m_elementAlignedSize * frameIndex;
        bufferInfo.range            = c->m_elementSize;

        writeSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_shader->m_context->m_device, 1, &writeSet, 0, nullptr);

        break;
    }

    case CONSTANT_RESOURCE_TYPE_CONSTANT_ARRAY:
    {
        auto c = static_cast<ConstantArray_IVulkan*>(resource.first);

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer           = c->m_buffer;
        bufferInfo.offset           = c->m_elementAlignedSize *  c->m_reservedElementCount * (c->m_perFrameData ? frameIndex : 0);
        bufferInfo.range            = c->m_elementAlignedSize;

        writeSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_shader->m_context->m_device, 1, &writeSet, 0, nullptr);

        for(auto& a : m_arrayAlignedElementSizes)
            if (a.second == resource.second)
            {
                a.first = c->m_elementAlignedSize;
                break;
            }

        break;
    }

    case CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:
    {
        auto c = static_cast<Texture_IVulkan*>(resource.first);

        VkDescriptorImageInfo imageInfo;
        imageInfo.sampler           = c->m_sampler;
        imageInfo.imageView         = c->m_imageView;
        imageInfo.imageLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writeSet.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_shader->m_context->m_device, 1, &writeSet, 0, nullptr);
        break;

    }

    default:
    {
        m_shader->m_logfile->print(RED, L"API ERROR: Unknown resource type!", MRN_DEBUG_INFO);
        throw std::exception();
    }
    }
}

VkDescriptorType moraine::ConstantSet_IVulkan::getVulkanDescriptorType(ConstantResourceType type)
{
    switch (type)
    {
    case CONSTANT_RESOURCE_TYPE_CONSTANT_BUFFER:            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case CONSTANT_RESOURCE_TYPE_CONSTANT_ARRAY:             return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER:             return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case CONSTANT_RESOURCE_TYPE_STORAGE_BUFFER_DYNAMIC:     return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:     return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    default:                                                return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}
