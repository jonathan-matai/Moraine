#include "mrn_shader_vk.h"

moraine::Shader_IVulkan::Shader_IVulkan(String shader, GraphicsContext context) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_logfile(m_context->m_logfile),
    m_pipeline(VK_NULL_HANDLE),
    m_layout(VK_NULL_HANDLE)
{
    std::ifstream configFile(shader.wcstr(), std::ifstream::binary);
    assert(m_logfile, configFile.is_open(), sprintf(L"Couldn't open Shader\"%s\"", shader.wcstr()), MRN_DEBUG_INFO);

    Json::Value jsonFile;
    configFile >> jsonFile;
    assert(m_logfile, jsonFile["fileType"].asString() == std::string("MORAINE_SHADER"), sprintf(L"Shader \"%s\" is not a shader config file!", shader.wcstr()), MRN_DEBUG_INFO);

    *(strrchr(shader.mbbuf(), '\\') + 1) = '\0';

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;

    if (jsonFile["vertexShader"]["spirvVulkan"].isString())
        compileShaderStage(std::string(shader.mbstr()) + jsonFile["vertexShader"]["spirvVulkan"].asString(), shaderStages, shaderModules, VK_SHADER_STAGE_VERTEX_BIT);

    if (jsonFile["fragmentShader"]["spirvVulkan"].isString())
        compileShaderStage(std::string(shader.mbstr()) + jsonFile["fragmentShader"]["spirvVulkan"].asString(), shaderStages, shaderModules, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineVertexInputStateCreateInfo vi_state;
    vi_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_state.pNext                              = nullptr;
    vi_state.flags                              = 0;
    vi_state.vertexBindingDescriptionCount      = 0;
    vi_state.pVertexBindingDescriptions         = nullptr;
    vi_state.vertexAttributeDescriptionCount    = 0;
    vi_state.pVertexAttributeDescriptions       = nullptr;

    VkPipelineInputAssemblyStateCreateInfo ia_state;
    ia_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_state.pNext                              = nullptr;
    ia_state.flags                              = 0;
    ia_state.topology                           = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; /// get topolpgy
    ia_state.primitiveRestartEnable             = VK_FALSE;

    uint32_t width = dynamic_cast<GraphicsContext_IVulkan*>(context.get())->m_swapchainWidth;
    uint32_t height = dynamic_cast<GraphicsContext_IVulkan*>(context.get())->m_swapchainHeight;

    VkViewport viewport;
    viewport.x                                  = 0;
    viewport.y                                  = 0;
    viewport.width                              = static_cast<float>(width);
    viewport.height                             = static_cast<float>(height);
    viewport.minDepth                           = 0.0f;
    viewport.maxDepth                           = 1.0f;

    VkRect2D scissor;
    scissor.offset                              = { 0, 0 };
    scissor.extent                              = { width, height };

    VkPipelineViewportStateCreateInfo vp_state;
    vp_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state.pNext                              = nullptr;
    vp_state.flags                              = 0;
    vp_state.viewportCount                      = 1;
    vp_state.pViewports                         = &viewport;
    vp_state.scissorCount                       = 1;
    vp_state.pScissors                          = &scissor;

    VkPipelineRasterizationStateCreateInfo rz_state;
    rz_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rz_state.pNext                              = nullptr;
    rz_state.flags                              = 0;
    rz_state.depthClampEnable                   = VK_FALSE;
    rz_state.rasterizerDiscardEnable            = VK_FALSE;
    rz_state.polygonMode                        = VK_POLYGON_MODE_FILL; /// get polygon mode
    rz_state.cullMode                           = VK_CULL_MODE_BACK_BIT; /// get cull mode
    rz_state.frontFace                          = VK_FRONT_FACE_CLOCKWISE;
    rz_state.depthBiasEnable                    = VK_FALSE;
    rz_state.depthBiasConstantFactor            = 0.0f;
    rz_state.depthBiasClamp                     = 0.0f;
    rz_state.depthBiasSlopeFactor               = 0.0f;
    rz_state.lineWidth                          = 5.0f; /// get line width

    VkPipelineMultisampleStateCreateInfo ms_state;
    ms_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_state.pNext                              = nullptr;
    ms_state.flags                              = 0;
    ms_state.rasterizationSamples               = VK_SAMPLE_COUNT_1_BIT; /// get msaa
    ms_state.sampleShadingEnable                = VK_FALSE; /// get ssaa
    ms_state.minSampleShading                   = 0.0f; /// get ssaa
    ms_state.pSampleMask                        = nullptr;
    ms_state.alphaToCoverageEnable              = VK_FALSE;
    ms_state.alphaToOneEnable                   = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo ds_state;
    ds_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds_state.pNext                              = nullptr;
    ds_state.flags                              = 0;
    ds_state.depthTestEnable                    = VK_FALSE; /// get depth test
    ds_state.depthWriteEnable                   = VK_FALSE; /// get depth test
    ds_state.depthCompareOp                     = VK_COMPARE_OP_LESS;
    ds_state.depthBoundsTestEnable              = VK_FALSE;
    ds_state.stencilTestEnable                  = VK_FALSE;
    ds_state.front                              = { };
    ds_state.back                               = { };
    ds_state.minDepthBounds                     = 0.0f;
    ds_state.maxDepthBounds                     = 1.0f;

    VkPipelineColorBlendAttachmentState cba_state;
    cba_state.blendEnable                       = VK_FALSE; /// blend enable
    cba_state.srcColorBlendFactor               = VK_BLEND_FACTOR_SRC_ALPHA;
    cba_state.dstColorBlendFactor               = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba_state.colorBlendOp                      = VK_BLEND_OP_ADD;
    cba_state.srcAlphaBlendFactor               = VK_BLEND_FACTOR_ONE;
    cba_state.dstAlphaBlendFactor               = VK_BLEND_FACTOR_ZERO;
    cba_state.alphaBlendOp                      = VK_BLEND_OP_ADD;
    cba_state.colorWriteMask                    = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb_state;
    cb_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_state.pNext                              = nullptr;
    cb_state.flags                              = 0;
    cb_state.logicOpEnable                      = VK_FALSE;
    cb_state.logicOp                            = VK_LOGIC_OP_NO_OP;
    cb_state.attachmentCount                    = 1;
    cb_state.pAttachments                       = &cba_state;
    cb_state.blendConstants[0]                  = 0.0f;
    cb_state.blendConstants[1]                  = 0.0f;
    cb_state.blendConstants[2]                  = 0.0f;
    cb_state.blendConstants[3]                  = 0.0f;

    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dy_state;
    dy_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dy_state.pNext                              = nullptr;
    dy_state.flags                              = 0;
    dy_state.dynamicStateCount                  = static_cast<uint32_t>(dynamicStates.size());
    dy_state.pDynamicStates                     = dynamicStates.data();

    VkPipelineLayoutCreateInfo vplci;
    vplci.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vplci.pNext                                 = nullptr;
    vplci.flags                                 = 0;
    vplci.setLayoutCount                        = 0;
    vplci.pSetLayouts                           = nullptr;
    vplci.pushConstantRangeCount                = 0;
    vplci.pPushConstantRanges                   = nullptr;

    assert_vulkan(m_logfile, vkCreatePipelineLayout(m_context->m_device, &vplci, nullptr, &m_layout), L"vkCreatePipelineLayout() failed", MRN_DEBUG_INFO);

    VkGraphicsPipelineCreateInfo vgpci;
    vgpci.sType                                 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vgpci.pNext                                 = nullptr;
    vgpci.flags                                 = 0;
    vgpci.stageCount                            = static_cast<uint32_t>(shaderStages.size());
    vgpci.pStages                               = shaderStages.data();
    vgpci.pVertexInputState                     = &vi_state;
    vgpci.pInputAssemblyState                   = &ia_state;
    vgpci.pTessellationState                    = nullptr;
    vgpci.pViewportState                        = &vp_state;
    vgpci.pRasterizationState                   = &rz_state;
    vgpci.pMultisampleState                     = &ms_state;
    vgpci.pDepthStencilState                    = &ds_state;
    vgpci.pColorBlendState                      = &cb_state;
    vgpci.pDynamicState                         = &dy_state;
    vgpci.layout                                = m_layout;
    vgpci.renderPass                            = m_context->m_renderPass;
    vgpci.subpass                               = 0;
    vgpci.basePipelineHandle                    = VK_NULL_HANDLE;
    vgpci.basePipelineIndex                     = 0;

    assert_vulkan(m_logfile, vkCreateGraphicsPipelines(m_context->m_device, VK_NULL_HANDLE, 1, &vgpci, nullptr, &m_pipeline), L"vkCreateGraphicsPipelines() failed", MRN_DEBUG_INFO);

    for (const auto& a : shaderModules)
        vkDestroyShaderModule(m_context->m_device, a, nullptr);
}

moraine::Shader_IVulkan::~Shader_IVulkan()
{
    vkDestroyPipeline(m_context->m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_context->m_device, m_layout, nullptr);
}

void moraine::Shader_IVulkan::compileShaderStage(std::string path, std::vector<VkPipelineShaderStageCreateInfo>& outStage, std::vector<VkShaderModule>& outModule, VkShaderStageFlagBits stage)
{
    std::ifstream binary(path.c_str(), std::ios::ate | std::ios::binary);

    assert(m_logfile, binary.is_open(), sprintf(L"Couldn't open file \"%s\"!", path.c_str()), MRN_DEBUG_INFO);

    size_t fileSize = static_cast<size_t>(binary.tellg());
    char* buffer = new char[fileSize];
    binary.seekg(0);
    binary.read(buffer, fileSize);

    binary.close();

    VkShaderModuleCreateInfo vsmci;
    vsmci.sType             = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vsmci.pNext             = nullptr;
    vsmci.flags             = 0;
    vsmci.codeSize          = fileSize;
    vsmci.pCode             = reinterpret_cast<const uint32_t*>(buffer);

    VkShaderModule module;

    assert_vulkan(m_logfile, vkCreateShaderModule(m_context->m_device, &vsmci, nullptr, &module), L"vkCreateShaderModule() failed", MRN_DEBUG_INFO);

    VkPipelineShaderStageCreateInfo vpssci;
    vpssci.sType                                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vpssci.pNext                                = nullptr;
    vpssci.flags                                = 0;
    vpssci.stage                                = stage;
    vpssci.module                               = module;
    vpssci.pName                                = "main";
    vpssci.pSpecializationInfo                  = nullptr;

    outStage.push_back(vpssci);
    outModule.push_back(module);

    delete[] buffer;
}
