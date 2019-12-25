#pragma once

#include "mrn_shader.h"

#include "mrn_gfxcontext_vk.h"
#include <json.h>

namespace moraine
{
    class Shader_IVulkan : public Shader_T
    {
    public:

        Shader_IVulkan(String shader, GraphicsContext context);
        ~Shader_IVulkan() override;

        void bind(VkCommandBuffer buffer);

        void compileShaderStage(Stringr path, std::vector<VkPipelineShaderStageCreateInfo>& outStage, std::vector<VkShaderModule>& outModule, VkShaderStageFlagBits stage);

        void createVertexInputState(Json::Value& jsonfile, std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes, Stringr fileName);
        void createPipelineLayout(Json::Value& jsonfile, Stringr fileName);

        VkFormat stringToVkFormat(const char* string);

        std::shared_ptr<GraphicsContext_IVulkan>        m_context;
        Logfile                                         m_logfile;
        VkPipeline                                      m_pipeline;
        std::vector<VkDescriptorSetLayout>              m_descriptorLayouts;
        std::vector<std::vector<VkDescriptorPoolSize>>  m_desriptorPoolSizes;
        VkPipelineLayout                                m_layout;
    };
}