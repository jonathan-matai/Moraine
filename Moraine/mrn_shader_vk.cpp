#include "mrn_shader_vk.h"

moraine::Shader_IVulkan::Shader_IVulkan(String shader, GraphicsContext context) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_logfile(m_context->m_logfile),
    m_pipeline(VK_NULL_HANDLE),
    m_layout(VK_NULL_HANDLE)
{
    Time start = Time::now();

    std::ifstream configFile(shader.wcstr(), std::ifstream::binary);
    assert(m_logfile, configFile.is_open(), sprintf(L"Couldn't open Shader\"%s\"", shader.wcstr()), MRN_DEBUG_INFO);

    Json::Value jsonFile;
    configFile >> jsonFile;
    assert(m_logfile, jsonFile["fileType"].asString() == std::string("MORAINE_SHADER"), sprintf(L"Shader \"%s\" is not a shader config file!", shader.wcstr()), MRN_DEBUG_INFO);

    configFile.close();

    String path = shader;
    *(strrchr(path.mbbuf(), '\\') + 1) = '\0';

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;

    if (jsonFile["vertexShader"]["spirvVulkan"].isString())
        compileShaderStage(std::string(path.mbstr()) + jsonFile["vertexShader"]["spirvVulkan"].asString(), shaderStages, shaderModules, VK_SHADER_STAGE_VERTEX_BIT);

    if (jsonFile["fragmentShader"]["spirvVulkan"].isString())
        compileShaderStage(std::string(path.mbstr()) + jsonFile["fragmentShader"]["spirvVulkan"].asString(), shaderStages, shaderModules, VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    createVertexInputState(jsonFile, bindings, attributes, shader);

    VkPipelineVertexInputStateCreateInfo vi_state;
    vi_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_state.pNext                              = nullptr;
    vi_state.flags                              = 0;
    vi_state.vertexBindingDescriptionCount      = static_cast<uint32_t>(bindings.size());
    vi_state.pVertexBindingDescriptions         = bindings.data();
    vi_state.vertexAttributeDescriptionCount    = static_cast<uint32_t>(attributes.size());
    vi_state.pVertexAttributeDescriptions       = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo ia_state;
    ia_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_state.pNext                              = nullptr;
    ia_state.flags                              = 0;
    ia_state.topology                           = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ia_state.primitiveRestartEnable             = VK_FALSE;

    if (jsonFile["topology"].isString())
    {
        auto topology = jsonFile["topology"].asString();

        if (topology == "pointList")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        else if (topology == "lineList")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        else if (topology == "lineStrip")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        else if (topology == "triangleList")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        else if (topology == "triangleStrip")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        else if (topology == "triangleFan")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        else if (topology == "lineListAdj")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
        else if (topology == "lineStripAdj")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
        else if (topology == "triangleListAdj")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
        else if (topology == "triangleStripAdj")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
        else if (topology == "patchList")
            ia_state.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        else
            m_logfile->print(YELLOW, sprintf(L"The argument \"%S\" for \"topology\" in shader config file \"%s\" is invalid! Using \"triangleList\" as default parameter", 
                             topology.c_str(), shader.wcstr()), MRN_DEBUG_INFO);
    }

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
    rz_state.polygonMode                        = VK_POLYGON_MODE_FILL;
    rz_state.cullMode                           = VK_CULL_MODE_BACK_BIT;
    rz_state.frontFace                          = VK_FRONT_FACE_CLOCKWISE;
    rz_state.depthBiasEnable                    = VK_FALSE;
    rz_state.depthBiasConstantFactor            = 0.0f;
    rz_state.depthBiasClamp                     = 0.0f;
    rz_state.depthBiasSlopeFactor               = 0.0f;
    rz_state.lineWidth                          = 5.0f;

    if (jsonFile["polygonMode"].isString())
    {
        auto polygonMode = jsonFile["polygonMode"].asString();

        if (polygonMode == "fill")
            rz_state.polygonMode = VK_POLYGON_MODE_FILL;
        else if (polygonMode == "line")
            rz_state.polygonMode = VK_POLYGON_MODE_LINE;
        else if (polygonMode == "point")
            rz_state.polygonMode = VK_POLYGON_MODE_POINT;
        else
            m_logfile->print(YELLOW, sprintf(L"The argument \"%S\" for \"polygonMode\" in shader config file \"%s\" is invalid! Using \"fill\" as default parameter",
                             polygonMode.c_str(), shader.wcstr()), MRN_DEBUG_INFO);
    }

    if (jsonFile["backfaceCulling"].isBool())
        rz_state.cullMode = jsonFile["backfaceCulling"].asBool() ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;

    if (jsonFile["lineWidth"].isDouble())
        rz_state.lineWidth = static_cast<float>(jsonFile["lineWidth"].asDouble());

    VkPipelineMultisampleStateCreateInfo ms_state;
    ms_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_state.pNext                              = nullptr;
    ms_state.flags                              = 0;
    ms_state.rasterizationSamples               = VK_SAMPLE_COUNT_1_BIT;
    ms_state.sampleShadingEnable                = VK_FALSE;
    ms_state.minSampleShading                   = 0.0f;
    ms_state.pSampleMask                        = nullptr;
    ms_state.alphaToCoverageEnable              = VK_FALSE;
    ms_state.alphaToOneEnable                   = VK_FALSE;

    if (jsonFile["multiSampling"].isDouble())
    {
        float multiSampling = static_cast<float>(jsonFile["multiSampling"].asDouble());

        if (multiSampling > 0.0f)
        {
            ms_state.sampleShadingEnable = VK_TRUE;
            ms_state.minSampleShading = clamp(0.0f, multiSampling, 1.0f);
        }
    }

    VkBool32 depthBuffering = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo ds_state;
    ds_state.sType                              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds_state.pNext                              = nullptr;
    ds_state.flags                              = 0;
    ds_state.depthTestEnable                    = depthBuffering;
    ds_state.depthWriteEnable                   = depthBuffering;
    ds_state.depthCompareOp                     = VK_COMPARE_OP_LESS;
    ds_state.depthBoundsTestEnable              = VK_FALSE;
    ds_state.stencilTestEnable                  = VK_FALSE;
    ds_state.front                              = { };
    ds_state.back                               = { };
    ds_state.minDepthBounds                     = 0.0f;
    ds_state.maxDepthBounds                     = 1.0f;

    if (jsonFile["depthTest"].isBool())
        depthBuffering = jsonFile["depthTest"].asBool() ? VK_TRUE : VK_FALSE;

    VkPipelineColorBlendAttachmentState cba_state;
    cba_state.blendEnable                       = VK_FALSE;
    cba_state.srcColorBlendFactor               = VK_BLEND_FACTOR_SRC_ALPHA;
    cba_state.dstColorBlendFactor               = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba_state.colorBlendOp                      = VK_BLEND_OP_ADD;
    cba_state.srcAlphaBlendFactor               = VK_BLEND_FACTOR_ONE;
    cba_state.dstAlphaBlendFactor               = VK_BLEND_FACTOR_ZERO;
    cba_state.alphaBlendOp                      = VK_BLEND_OP_ADD;
    cba_state.colorWriteMask                    = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    if (jsonFile["blending"].isBool())
        cba_state.blendEnable = jsonFile["blending"].asBool() ? VK_TRUE : VK_FALSE;

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

    //VkPipelineLayoutCreateInfo vplci;
    //vplci.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    //vplci.pNext                                 = nullptr;
    //vplci.flags                                 = 0;
    //vplci.setLayoutCount                        = 0;
    //vplci.pSetLayouts                           = nullptr;
    //vplci.pushConstantRangeCount                = 0;
    //vplci.pPushConstantRanges                   = nullptr;
    //
    //assert_vulkan(m_logfile, vkCreatePipelineLayout(m_context->m_device, &vplci, nullptr, &m_layout), L"vkCreatePipelineLayout() failed", MRN_DEBUG_INFO);

    createPipelineLayout(jsonFile, shader);

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

    m_logfile->print(WHITE, sprintf(L"Created shader \"%s\" (%.3f ms)", shader.wcstr(), Time::duration(start, Time::now()).getMillisecondsF()));
}

moraine::Shader_IVulkan::~Shader_IVulkan()
{
    for (const auto& a : m_descriptorLayouts)
        vkDestroyDescriptorSetLayout(m_context->m_device, a, nullptr);

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


void moraine::Shader_IVulkan::createVertexInputState(Json::Value& jsonfile, std::vector<VkVertexInputBindingDescription>& bindings, std::vector<VkVertexInputAttributeDescription>& attributes, Stringr fileName)
{
    if (not jsonfile["vertexBindings"].isArray())
    {
        m_logfile->print(GREY, sprintf(L"Shader \"%s\" does not contain any vertex bindings!", fileName.wcstr()), MRN_DEBUG_INFO);
        return;
    }

    const Json::Value& vertexBindings = jsonfile["vertexBindings"];

    uint32_t location = 0;

    for (size_t i = 0; i < vertexBindings.size(); ++i)
    {
        VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;
        uint32_t stride = 0;

        if (vertexBindings[static_cast<int>(i)]["inputRate"].isString())
        {
            auto inputRate = vertexBindings[static_cast<int>(i)]["inputRate"].asString();

            if (inputRate == "vertex")
                rate = VK_VERTEX_INPUT_RATE_VERTEX;
            else if (inputRate == "instance")
                rate = VK_VERTEX_INPUT_RATE_INSTANCE;
            else
                m_logfile->print(YELLOW, sprintf(L"The value \"%S\" for \"vertexBindings[%d].inputRate\" in shader config file \"%s\" is invalid! Using \"vertex\" as default parameter",
                                 inputRate.c_str(), i, fileName.wcstr()), MRN_DEBUG_INFO);
        }

        if (vertexBindings[static_cast<int>(i)]["stride"].isIntegral())
            stride = vertexBindings[static_cast<int>(i)]["stride"].asUInt();
        else
            m_logfile->print(YELLOW, sprintf(L"The value for \"vertexBindings[%d].stride\" in shader config file \"%s\" is invalid or missing! Using 0 as default parameter",
                             i, fileName.wcstr()), MRN_DEBUG_INFO);

        bindings.push_back({ static_cast<uint32_t>(i), stride, rate });

        if (not vertexBindings[static_cast<int>(i)]["locations"].isArray())
        {
            m_logfile->print(YELLOW, sprintf(L"The value for \"vertexBindings[%d].locations\" in shader config file \"%s\" is invalid or missing!", i, fileName.wcstr()), MRN_DEBUG_INFO);
            continue;
        }

        const Json::Value& vertexAttributes = vertexBindings[static_cast<int>(i)]["locations"];

        for (size_t j = 0; j < vertexAttributes.size(); ++j)
        {
            VkFormat format = VK_FORMAT_UNDEFINED;
            uint32_t offset = 0;

            if (vertexAttributes[static_cast<int>(j)]["type"].isString())
            {
                auto type = vertexAttributes[static_cast<int>(j)]["type"].asString();
                format = stringToVkFormat(type.c_str());

                if (format == VK_FORMAT_UNDEFINED)
                {
                    m_logfile->print(YELLOW, sprintf(L"The value \"%S\" for \"vertexBindings[%d].locations[%d].type\" in shader config file \"%s\" is invalid! Using \"float4\" as default parameter", 
                                     type.c_str(), i, j, fileName.wcstr()), MRN_DEBUG_INFO);
                    continue;
                }   
            }
            else
            {
                m_logfile->print(YELLOW, sprintf(L"The value for \"vertexBindings[%d].locations[%d].type\" in shader config file \"%s\" is invalid or missing!", i, j, fileName.wcstr()), MRN_DEBUG_INFO);
                continue;
            }

            if (vertexAttributes[static_cast<int>(j)]["offset"].isIntegral())
                offset = vertexAttributes[static_cast<int>(j)]["offset"].asUInt();
            else
                m_logfile->print(YELLOW, sprintf(L"The value for \"vertexBindings[%d].locations[%d].offset\" in shader config file \"%s\" is invalid or missing! Using 0 as defualt parameter", i, j, fileName.wcstr()), MRN_DEBUG_INFO);

            attributes.push_back({ location, static_cast<uint32_t>(i), format, offset });
            ++location;
        }
    }
}


void moraine::Shader_IVulkan::createPipelineLayout(Json::Value& jsonfile, Stringr fileName)
{
    if (not jsonfile["descriptorBindings"].isArray())
    {
        m_logfile->print(GREY, sprintf(L"Shader \"%s\" does not contain any descriptor bindings!", fileName.wcstr()), MRN_DEBUG_INFO);
        goto makePipelineLayout;
    }

    {
        const Json::Value& descriptorSets = jsonfile["descriptorBindings"];

        m_descriptorLayouts.resize(descriptorSets.size());

        for (size_t i = 0; i < descriptorSets.size(); ++i)
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            if (descriptorSets[static_cast<int>(i)].isArray())
            {
                const Json::Value& descriptorBindings = descriptorSets[static_cast<int>(i)];

                for (size_t j = 0; j < descriptorBindings.size(); ++j)
                {
                    VkDescriptorSetLayoutBinding binding;
                    binding.binding                     = static_cast<uint32_t>(j);
                    binding.descriptorType              = VK_DESCRIPTOR_TYPE_MAX_ENUM;
                    binding.descriptorCount             = 1;
                    binding.stageFlags                  = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
                    binding.pImmutableSamplers          = nullptr;

                    if (descriptorBindings[static_cast<int>(j)]["type"].isString())
                    {
                        auto type = descriptorBindings[static_cast<int>(j)]["type"].asString();

                        if (type == "constantBuffer")
                            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        else if (type == "constantArray")
                            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                        else if (type == "combinedImageSampler")
                            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        else if (type == "storageBuffer")
                            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        else if (type == "storageArray")
                            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                        else
                        {
                            m_logfile->print(YELLOW, sprintf(L"The value \"%S\" for \"descriptorBindings[%d][%d].type\" in shader config file \"%s\" is invalid!", type.c_str(), i, j, fileName.wcstr()), MRN_DEBUG_INFO);
                            continue;
                        }
                    }
                    else
                    {
                        m_logfile->print(YELLOW, sprintf(L"The value for \"descriptorBindings[%d][%d].type\" in shader config file \"%s\" is invalid or missing!", i, j, fileName.wcstr()), MRN_DEBUG_INFO);
                        continue;
                    }

                    if (descriptorBindings[static_cast<int>(j)]["elementCount"].isIntegral())
                        binding.descriptorCount = descriptorBindings[static_cast<int>(j)]["elementCount"].asUInt();

                    if (descriptorBindings[static_cast<int>(j)]["stage"].isString())
                    {
                        auto stage = descriptorBindings[static_cast<int>(j)]["stage"].asString();

                        if (stage == "vertexShader")
                            binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                        else if (stage == "fragmentShader")
                            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                        else if (stage == "computeShader")
                            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                        else
                        {
                            m_logfile->print(YELLOW, sprintf(L"The value \"%S\" for \"descriptorBindings[%d][%d].stage\" in shader config file \"%s\" is invalid!", stage.c_str(), i, j, fileName.wcstr()), MRN_DEBUG_INFO);
                            continue;
                        }
                    }
                    else
                    {
                        m_logfile->print(YELLOW, sprintf(L"The value for \"descriptorBindings[%d][%d].stage\" in shader config file \"%s\" is invalid or missing!", i, j, fileName.wcstr()), MRN_DEBUG_INFO);
                        continue;
                    }

                    bindings.push_back(binding);
                }
            }
            else
            {
                m_logfile->print(YELLOW, sprintf(L"The value for \"descriptorBindings[%d]\" in shader config file \"%s\" is invalid!", i, fileName.wcstr()), MRN_DEBUG_INFO);
            }

            VkDescriptorSetLayoutCreateInfo vdslci;
            vdslci.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            vdslci.pNext                           = nullptr;
            vdslci.flags                           = 0;
            vdslci.bindingCount                    = static_cast<uint32_t>(bindings.size());
            vdslci.pBindings                       = bindings.data();

            assert_vulkan(m_context->m_logfile, vkCreateDescriptorSetLayout(m_context->m_device, &vdslci, nullptr, &m_descriptorLayouts[i]), L"vkCreateDescriptorSetLayout() failed", MRN_DEBUG_INFO);
        }
    }
    
    makePipelineLayout:

    VkPipelineLayoutCreateInfo vplci;
    vplci.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vplci.pNext                                 = nullptr;
    vplci.flags                                 = 0;
    vplci.setLayoutCount                        = static_cast<uint32_t>(m_descriptorLayouts.size());
    vplci.pSetLayouts                           = m_descriptorLayouts.data();
    vplci.pushConstantRangeCount                = 0;
    vplci.pPushConstantRanges                   = nullptr;

    assert_vulkan(m_logfile, vkCreatePipelineLayout(m_context->m_device, &vplci, nullptr, &m_layout), L"vkCreatePipelineLayout() failed", MRN_DEBUG_INFO);
}


VkFormat moraine::Shader_IVulkan::stringToVkFormat(const char* string)
{
    if (strncmp(string, "int", 3) == 0)
    {
        if (string[3] == '\0')
            return VK_FORMAT_R32_SINT;

        switch (string[3] - '0')
        {
        case 1:  return VK_FORMAT_R32_SINT;
        case 2:  return VK_FORMAT_R32G32_SINT;
        case 3:  return VK_FORMAT_R32G32B32_SINT;
        case 4:  return VK_FORMAT_R32G32B32A32_SINT;
        default: return VK_FORMAT_UNDEFINED;
        }
    }
    else if (strncmp(string, "uint", 4) == 0)
    {
        if (string[4] == '\0')
            return VK_FORMAT_R32_UINT;

        switch (string[4] - '0')
        {
        case 1:  return VK_FORMAT_R32_UINT;
        case 2:  return VK_FORMAT_R32G32_UINT;
        case 3:  return VK_FORMAT_R32G32B32_UINT;
        case 4:  return VK_FORMAT_R32G32B32A32_UINT;
        default: return VK_FORMAT_UNDEFINED;
        }
    }
    else if (strncmp(string, "float", 5) == 0)
    {
        if (string[5] == '\0')
            return VK_FORMAT_R32_SFLOAT;

        switch (string[5] - '0')
        {
        case 1:  return VK_FORMAT_R32_SFLOAT;
        case 2:  return VK_FORMAT_R32G32_SFLOAT;
        case 3:  return VK_FORMAT_R32G32B32_SFLOAT;
        case 4:  return VK_FORMAT_R32G32B32A32_SFLOAT;
        default: return VK_FORMAT_UNDEFINED;
        }
    }

    return VK_FORMAT_UNDEFINED;
}
