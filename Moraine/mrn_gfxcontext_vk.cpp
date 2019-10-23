#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "mrn_gfxcontext_vk.h"

#pragma comment(lib, "vulkan-1.lib")

moraine::GraphicsContext_IVulkan::GraphicsContext_IVulkan(const GraphicsContextDesc& desc, Logfile logfile, Window window) :
    m_description(desc),
    m_instance(0),
    m_messenger(0),
    m_logfile(logfile),
    m_window(window)
{
    createVulkanInstance();
    createVulkanSurface();
    createPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createRenderPass();
    createFrameBuffers();

    VmaAllocatorCreateInfo vmaaci = { };
    vmaaci.device = m_device;
    vmaaci.physicalDevice = m_physicalDevice.device;

    assert_vulkan(m_logfile, vmaCreateAllocator(&vmaaci, &m_allocator), L"vmaCreateAllocator() failed", MRN_DEBUG_INFO);
}


moraine::GraphicsContext_IVulkan::~GraphicsContext_IVulkan()
{
    vmaDestroyAllocator(m_allocator);

    for (const auto& a : m_frameBuffers)
        vkDestroyFramebuffer(m_device, a, nullptr);

    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (const auto& a : m_swapchainImageViews)
        vkDestroyImageView(m_device, a, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    for (const auto& a : m_mainThreadCommandPools)
        if(a != VK_NULL_HANDLE)
            vkDestroyCommandPool(m_device, a, nullptr);

    vkDestroyDevice(m_device, nullptr);

    vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);

    if (m_messenger)
    {
        auto DestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));

        if (DestroyDebugUtilsMessengerEXT)
            DestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
    }

    vkDestroyInstance(m_instance, nullptr);
}


void moraine::GraphicsContext_IVulkan::createVulkanInstance()
{
    Time start = Time::now();

    VkApplicationInfo vai;
    vai.sType                               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vai.pNext                               = nullptr;
    vai.pApplicationName                    = m_description.applicationName.mbstr();
    vai.applicationVersion                  = VK_MAKE_VERSION(1, 0, 0);
    vai.pEngineName                         = "Moraine Graphics Libraray";
    vai.engineVersion                       = VK_MAKE_VERSION(0, 0, 1010);
    vai.apiVersion                          = VK_VERSION_1_1;

    std::vector<String> requestedLayers;

    if (m_description.enableValidation)
    {
        requestedLayers.push_back("VK_LAYER_KHRONOS_validation");
        requestedLayers.push_back("VK_LAYER_LUNARG_monitor");
    }

    auto enabledLayers = listAndEnableInstanceLayers(requestedLayers);
    
    std::vector<String> requestedExtensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };

    if (m_description.enableValidation)
        requestedExtensions.push_back("VK_EXT_debug_utils");

    auto enabledExtensions = listAndEnableInstanceExtensions(requestedExtensions);

    VkInstanceCreateInfo vici = {};
    vici.sType                              = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vici.pNext                              = nullptr;
    vici.flags                              = 0;
    vici.pApplicationInfo                   = &vai;
    vici.enabledLayerCount                  = static_cast<uint32_t>(enabledLayers.size());
    vici.ppEnabledLayerNames                = enabledLayers.data();
    vici.enabledExtensionCount              = static_cast<uint32_t>(enabledExtensions.size());
    vici.ppEnabledExtensionNames            = enabledExtensions.data();

    assert_vulkan(m_logfile, vkCreateInstance(&vici, nullptr, &m_instance), L"vkCreateInstance() failed!", MRN_DEBUG_INFO);

    m_logfile->print(WHITE, sprintf(L"Created VkInstance (%.3f ms)", Time::duration(start, Time::now()).getMillisecondsF()));

    if (m_description.enableValidation)
        setupValidation();
}


void moraine::GraphicsContext_IVulkan::setupValidation()
{
    Time start = Time::now();

    auto CreateDebugUtilsMessengerEXT = 
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));

    VkDebugUtilsMessengerCreateInfoEXT vdumci;
    vdumci.sType                            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    vdumci.pNext                            = nullptr;
    vdumci.flags                            = 0;
    vdumci.messageSeverity                  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    vdumci.messageType                      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    vdumci.pfnUserCallback                  = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                 void* pUserData)
                                            {
                                                return static_cast<GraphicsContext_IVulkan*>(pUserData)->debugCallback(messageSeverity, 
                                                                                                                       messageType, 
                                                                                                                       pCallbackData);
                                            };
    vdumci.pUserData                        = this;

    if (CreateDebugUtilsMessengerEXT)
        assert_vulkan(m_logfile, CreateDebugUtilsMessengerEXT(m_instance, &vdumci, nullptr, &m_messenger), L"vkCreateDebugUtilsMessengerEXT() failed", MRN_DEBUG_INFO);
    else
        assert_vulkan(m_logfile, VK_ERROR_EXTENSION_NOT_PRESENT, L"vkGetInstanceProcAddr(...,\"vkCreateDebugUtilsMessengerEXT\") failed", MRN_DEBUG_INFO);

    m_logfile->print(WHITE, moraine::sprintf(L"Created VkDebugUtilsMessenger (%.3f ms)", Time::duration(start, Time::now()).getMillisecondsF()));
}


void moraine::GraphicsContext_IVulkan::createPhysicalDevice()
{
    uint32_t n_devices;
    assert_vulkan(m_logfile, vkEnumeratePhysicalDevices(m_instance, &n_devices, nullptr), L"vkEnumeratePhysicalDevices() failed", MRN_DEBUG_INFO);

    if (n_devices == 0)
    {
        m_logfile->print({ 0xff, 0x00, 0x00 }, L"Couldn't find any devices supporting Vulkan!", MRN_DEBUG_INFO);
        throw std::exception();
    }

    std::vector<VkPhysicalDevice> p_devices(n_devices);
    assert_vulkan(m_logfile, vkEnumeratePhysicalDevices(m_instance, &n_devices, p_devices.data()), L"vkEnumeratePhysicalDevices() failed", MRN_DEBUG_INFO);

    std::vector<PhysicalDevice> deviceSpecs(n_devices);

    int bestDevice = -1;
    size_t bestScore = 0;

    for (uint32_t i = 0; i < n_devices; ++i)
    {
        size_t score = 0;

        deviceSpecs[i].device = p_devices[i];

        vkGetPhysicalDeviceProperties(p_devices[i], &deviceSpecs[i].deviceProperties);
        vkGetPhysicalDeviceFeatures(p_devices[i], &deviceSpecs[i].deviceFeatures);
        vkGetPhysicalDeviceMemoryProperties(p_devices[i], &deviceSpecs[i].memoryProperties);

        uint32_t n_queueFamilies;
        vkGetPhysicalDeviceQueueFamilyProperties(p_devices[i], &n_queueFamilies, nullptr);
        deviceSpecs[i].queueFamilyProperties.resize(n_queueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(p_devices[i], &n_queueFamilies, deviceSpecs[i].queueFamilyProperties.data());

        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_devices[i], m_windowSurface, &deviceSpecs[i].surfaceProperites) != VK_SUCCESS)
            m_logfile->print(YELLOW, sprintf(L"vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed for GPU %S", deviceSpecs[i].deviceProperties.deviceName), MRN_DEBUG_INFO);

        if (deviceSpecs[i].deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 10000;

        for (size_t j = 0; j < deviceSpecs[i].memoryProperties.memoryHeapCount; ++j)
            if(deviceSpecs[i].memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    score += deviceSpecs[i].memoryProperties.memoryHeaps[j].size / 1000000;

        if (score > bestScore)
        {
            bestScore = score;
            bestDevice = static_cast<int>(i);
        }
    }

    if (bestDevice != -1)
        m_physicalDevice = std::move(deviceSpecs[bestDevice]);
    else
    {
        m_logfile->print(RED, L"No suiting Vulkan Graphics Card found!");
        throw std::exception();
    }

    if (m_description.enableValidation)
    {
        std::vector<String> names = { L"Device Name" };

        for (const auto& a : deviceSpecs)
            names.push_back(a.deviceProperties.deviceName);

        Table table = createTable(L"Graphics Cards", WHITE, names);

        table->addColumn(WHITE, 
                         {
                             L"Vulkan Version",
                             L"Device Type",
                             L"DEVICE LIMITS",
                             L"-  maxImageDimension1D",
                             L"-  maxImageDimension2D",
                             L"-  maxImageDimension3D",
                             L"-  maxImageDimensionCube",
                             L"-  maxImageArrayLayers",
                             L"-  maxTexelBufferElements",
                             L"-  maxUniformBufferRange",
                             L"-  maxStorageBufferRange",
                             L"-  maxPushConstantsSize",
                             L"-  maxMemoryAllocationCount",
                             L"-  maxSamplerAllocationCount",
                             L"-  bufferImageGranularity",
                             L"-  sparseAddressSpaceSize",
                             L"-  maxBoundDescriptorSets",
                             L"-  maxPerStageDescriptorSamplers",
                             L"-  maxPerStageDescriptorUniformBuffers",
                             L"-  maxPerStageDescriptorStorageBuffers",
                             L"-  maxPerStageDescriptorSampledImages",
                             L"-  maxPerStageDescriptorStorageImages",
                             L"-  maxPerStageDescriptorInputAttachments",
                             L"-  maxPerStageResources",
                             L"-  maxDescriptorSetSamplers",
                             L"-  maxDescriptorSetUniformBuffers",
                             L"-  maxDescriptorSetUniformBuffersDynamic",
                             L"-  maxDescriptorSetStorageBuffers",
                             L"-  maxDescriptorSetStorageBuffersDynamic",
                             L"-  maxDescriptorSetSampledImages",
                             L"-  maxDescriptorSetStorageImages",
                             L"-  maxDescriptorSetInputAttachments",
                             L"-  maxVertexInputAttributes",
                             L"-  maxVertexInputBindings",
                             L"-  maxVertexInputAttributeOffset",
                             L"-  maxVertexInputBindingStride",
                             L"-  maxVertexOutputComponents",
                             L"-  maxTessellationGenerationLevel",
                             L"-  maxTessellationPatchSize",
                             L"-  maxTessellationControlPerVertexInputComponents",
                             L"-  maxTessellationControlPerVertexOutputComponents",
                             L"-  maxTessellationControlPerPatchOutputComponents",
                             L"-  maxTessellationControlTotalOutputComponents",
                             L"-  maxTessellationEvaluationInputComponents",
                             L"-  maxTessellationEvaluationOutputComponents",
                             L"-  maxGeometryShaderInvocations",
                             L"-  maxGeometryInputComponents",
                             L"-  maxGeometryOutputComponents",
                             L"-  maxGeometryOutputVertices",
                             L"-  maxGeometryTotalOutputComponents",
                             L"-  maxFragmentInputComponents",
                             L"-  maxFragmentOutputAttachments",
                             L"-  maxFragmentDualSrcAttachments",
                             L"-  maxFragmentCombinedOutputResources",
                             L"-  maxComputeSharedMemorySize",
                             L"-  maxComputeWorkGroupCount[3]",
                             L"-  maxComputeWorkGroupInvocations",
                             L"-  maxComputeWorkGroupSize[3]",
                             L"-  subPixelPrecisionBits",
                             L"-  subTexelPrecisionBits",
                             L"-  mipmapPrecisionBits",
                             L"-  maxDrawIndexedIndexValue",
                             L"-  maxDrawIndirectCount",
                             L"-  maxSamplerLodBias",
                             L"-  maxSamplerAnisotropy",
                             L"-  maxViewports",
                             L"-  maxViewportDimensions[2]",
                             L"-  viewportBoundsRange[2]",
                             L"-  viewportSubPixelBits",
                             L"-  minMemoryMapAlignment",
                             L"-  minTexelBufferOffsetAlignment",
                             L"-  minUniformBufferOffsetAlignment",
                             L"-  minStorageBufferOffsetAlignment",
                             L"-  minTexelOffset",
                             L"-  maxTexelOffset",
                             L"-  minTexelGatherOffset",
                             L"-  maxTexelGatherOffset",
                             L"-  minInterpolationOffset",
                             L"-  maxInterpolationOffset",
                             L"-  subPixelInterpolationOffsetBits",
                             L"-  maxFramebufferWidth",
                             L"-  maxFramebufferHeight",
                             L"-  maxFramebufferLayers",
                             L"-  framebufferColorSampleCounts",
                             L"-  framebufferDepthSampleCounts",
                             L"-  framebufferStencilSampleCounts",
                             L"-  framebufferNoAttachmentsSampleCounts",
                             L"-  maxColorAttachments",
                             L"-  sampledImageColorSampleCounts",
                             L"-  sampledImageIntegerSampleCounts",
                             L"-  sampledImageDepthSampleCounts",
                             L"-  sampledImageStencilSampleCounts",
                             L"-  storageImageSampleCounts",
                             L"-  maxSampleMaskWords",
                             L"-  timestampComputeAndGraphics",
                             L"-  timestampPeriod",
                             L"-  maxClipDistances",
                             L"-  maxCullDistances",
                             L"-  maxCombinedClipAndCullDistances",
                             L"-  discreteQueuePriorities",
                             L"-  pointSizeRange[2]",
                             L"-  lineWidthRange[2]",
                             L"-  pointSizeGranularity",
                             L"-  lineWidthGranularity",
                             L"-  strictLines",
                             L"-  standardSampleLocations",
                             L"-  optimalBufferCopyOffsetAlignment",
                             L"-  optimalBufferCopyRowPitchAlignment",
                             L"-  nonCoherentAtomSize",
                             L"DEVICE FEATURES",
                             L"-  robustBufferAccess",
                             L"-  fullDrawIndexUint32",
                             L"-  imageCubeArray",
                             L"-  independentBlend",
                             L"-  geometryShader",
                             L"-  tessellationShader",
                             L"-  sampleRateShading",
                             L"-  dualSrcBlend",
                             L"-  logicOp",
                             L"-  multiDrawIndirect",
                             L"-  drawIndirectFirstInstance",
                             L"-  depthClamp",
                             L"-  depthBiasClamp",
                             L"-  fillModeNonSolid",
                             L"-  depthBounds",
                             L"-  wideLines",
                             L"-  largePoints",
                             L"-  alphaToOne",
                             L"-  multiViewport",
                             L"-  samplerAnisotropy",
                             L"-  textureCompressionETC2",
                             L"-  textureCompressionASTC_LDR",
                             L"-  textureCompressionBC",
                             L"-  occlusionQueryPrecise",
                             L"-  pipelineStatisticsQuery",
                             L"-  vertexPipelineStoresAndAtomics",
                             L"-  fragmentStoresAndAtomics",
                             L"-  shaderTessellationAndGeometryPointSize",
                             L"-  shaderImageGatherExtended",
                             L"-  shaderStorageImageExtendedFormats",
                             L"-  shaderStorageImageMultisample",
                             L"-  shaderStorageImageReadWithoutFormat",
                             L"-  shaderStorageImageWriteWithoutFormat",
                             L"-  shaderUniformBufferArrayDynamicIndexing",
                             L"-  shaderSampledImageArrayDynamicIndexing",
                             L"-  shaderStorageBufferArrayDynamicIndexing",
                             L"-  shaderStorageImageArrayDynamicIndexing",
                             L"-  shaderClipDistance",
                             L"-  shaderCullDistance",
                             L"-  shaderFloat64",
                             L"-  shaderInt64",
                             L"-  shaderInt16",
                             L"-  shaderResourceResidency",
                             L"-  shaderResourceMinLod",
                             L"-  sparseBinding",
                             L"-  sparseResidencyBuffer",
                             L"-  sparseResidencyImage2D",
                             L"-  sparseResidencyImage3D",
                             L"-  sparseResidency2Samples",
                             L"-  sparseResidency4Samples",
                             L"-  sparseResidency8Samples",
                             L"-  sparseResidency16Samples",
                             L"-  sparseResidencyAliased",
                             L"-  variableMultisampleRate",
                             L"-  inheritedQueries",
                         });

        for (const auto& a : deviceSpecs)
        {
            table->addColumn({
                                { WHITE, moraine::sprintf(L"%d.%d.%d", VK_VERSION_MAJOR(a.deviceProperties.apiVersion),
                                                                       VK_VERSION_MINOR(a.deviceProperties.apiVersion),
                                                                       VK_VERSION_PATCH(a.deviceProperties.apiVersion)) },
                                { WHITE, a.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU      ? L"discrete" :
                                        (a.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU    ? L"integrated" :
                                        (a.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU       ? L"virtual" : L"other")) },
                                { WHITE, L"" },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxImageDimension1D) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxImageDimension2D) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxImageDimension3D) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxImageDimensionCube) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxImageArrayLayers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTexelBufferElements) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxUniformBufferRange) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxStorageBufferRange) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPushConstantsSize) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxMemoryAllocationCount) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxSamplerAllocationCount) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.bufferImageGranularity) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.sparseAddressSpaceSize) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxBoundDescriptorSets) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageDescriptorSamplers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageDescriptorUniformBuffers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageDescriptorStorageBuffers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageDescriptorSampledImages) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageDescriptorStorageImages) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageDescriptorInputAttachments) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxPerStageResources) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetSamplers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetUniformBuffers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetStorageBuffers) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetStorageBuffersDynamic) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetSampledImages) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetStorageImages) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDescriptorSetInputAttachments) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxVertexInputAttributes) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxVertexInputBindings) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxVertexInputAttributeOffset) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxVertexInputBindingStride) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxVertexOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationGenerationLevel) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationPatchSize) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationControlPerVertexInputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationControlPerVertexOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationControlPerPatchOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationControlTotalOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationEvaluationInputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTessellationEvaluationOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxGeometryShaderInvocations) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxGeometryInputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxGeometryOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxGeometryOutputVertices) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxGeometryTotalOutputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFragmentInputComponents) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFragmentOutputAttachments) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFragmentDualSrcAttachments) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFragmentCombinedOutputResources) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxComputeSharedMemorySize) },
                                { WHITE, moraine::sprintf(L"%u x %u x %u", a.deviceProperties.limits.maxComputeWorkGroupCount[0], a.deviceProperties.limits.maxComputeWorkGroupCount[1], a.deviceProperties.limits.maxComputeWorkGroupCount[2]) },
                                { WHITE, moraine::sprintf(L"%d", a.deviceProperties.limits.maxComputeWorkGroupInvocations) },
                                { WHITE, moraine::sprintf(L"%u x %u x %u", a.deviceProperties.limits.maxComputeWorkGroupSize[0], a.deviceProperties.limits.maxComputeWorkGroupSize[1], a.deviceProperties.limits.maxComputeWorkGroupSize[2]) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.subPixelPrecisionBits).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.subTexelPrecisionBits).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.mipmapPrecisionBits).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDrawIndexedIndexValue) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxDrawIndirectCount) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.maxSamplerLodBias) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.maxSamplerAnisotropy) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxViewports) },
                                { WHITE, moraine::sprintf(L"%u x %u", a.deviceProperties.limits.maxViewportDimensions[0], a.deviceProperties.limits.maxViewportDimensions[1]) },
                                { WHITE, moraine::sprintf(L"%f - %f", a.deviceProperties.limits.viewportBoundsRange[0], a.deviceProperties.limits.viewportBoundsRange[1]) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.viewportSubPixelBits).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.minMemoryMapAlignment) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.minTexelBufferOffsetAlignment) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.minUniformBufferOffsetAlignment) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.minStorageBufferOffsetAlignment) },
                                { WHITE, moraine::sprintf(L"%i", a.deviceProperties.limits.minTexelOffset) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTexelOffset) },
                                { WHITE, moraine::sprintf(L"%i", a.deviceProperties.limits.minTexelGatherOffset) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxTexelGatherOffset) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.minInterpolationOffset) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.maxInterpolationOffset) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.subPixelInterpolationOffsetBits).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFramebufferWidth) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFramebufferHeight) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxFramebufferLayers) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.framebufferColorSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.framebufferDepthSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.framebufferStencilSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.framebufferNoAttachmentsSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"%d", a.deviceProperties.limits.maxColorAttachments) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.sampledImageColorSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.sampledImageIntegerSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.sampledImageDepthSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.sampledImageStencilSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"0b%S", std::bitset<16>(a.deviceProperties.limits.storageImageSampleCounts).to_string().c_str()) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxSampleMaskWords) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.timestampComputeAndGraphics) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.timestampPeriod) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxClipDistances) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxCullDistances) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.maxCombinedClipAndCullDistances) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.discreteQueuePriorities) },
                                { WHITE, moraine::sprintf(L"%f - %f", a.deviceProperties.limits.pointSizeRange[0], a.deviceProperties.limits.pointSizeRange[1]) },
                                { WHITE, moraine::sprintf(L"%f - %f", a.deviceProperties.limits.lineWidthRange[0], a.deviceProperties.limits.lineWidthRange[1]) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.pointSizeGranularity) },
                                { WHITE, moraine::sprintf(L"%f", a.deviceProperties.limits.lineWidthGranularity) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.strictLines) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.standardSampleLocations) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.optimalBufferCopyOffsetAlignment) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.optimalBufferCopyRowPitchAlignment) },
                                { WHITE, moraine::sprintf(L"%u", a.deviceProperties.limits.nonCoherentAtomSize) },
                                { WHITE, L"" },
                                a.deviceFeatures.robustBufferAccess                         == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.fullDrawIndexUint32                        == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.imageCubeArray                             == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.independentBlend                           == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.geometryShader                             == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.tessellationShader                         == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sampleRateShading                          == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.dualSrcBlend                               == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.logicOp                                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.multiDrawIndirect                          == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.drawIndirectFirstInstance                  == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.depthClamp                                 == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.depthBiasClamp                             == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.fillModeNonSolid                           == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.depthBounds                                == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.wideLines                                  == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.largePoints                                == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.alphaToOne                                 == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.multiViewport                              == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.samplerAnisotropy                          == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.textureCompressionETC2                     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.textureCompressionASTC_LDR                 == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.textureCompressionBC                       == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.occlusionQueryPrecise                      == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.pipelineStatisticsQuery                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.vertexPipelineStoresAndAtomics             == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.fragmentStoresAndAtomics                   == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderTessellationAndGeometryPointSize     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderImageGatherExtended                  == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderStorageImageExtendedFormats          == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderStorageImageMultisample              == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderStorageImageReadWithoutFormat        == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderStorageImageWriteWithoutFormat       == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderUniformBufferArrayDynamicIndexing    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderSampledImageArrayDynamicIndexing     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderStorageBufferArrayDynamicIndexing    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderStorageImageArrayDynamicIndexing     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderClipDistance                         == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderCullDistance                         == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderFloat64                              == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderInt64                                == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderInt16                                == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderResourceResidency                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.shaderResourceMinLod                       == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseBinding                              == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidencyBuffer                      == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidencyImage2D                     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidencyImage3D                     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidency2Samples                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidency4Samples                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidency8Samples                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidency16Samples                   == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.sparseResidencyAliased                     == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.variableMultisampleRate                    == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                                a.deviceFeatures.inheritedQueries                           == VK_TRUE ? std::make_pair(GREEN, L"Yes") : std::make_pair(RED, L"No"),
                             });
                
        }

        m_logfile->print(table);
    }
}


void moraine::GraphicsContext_IVulkan::createLogicalDevice()
{
    Time start = Time::now();

    VkPhysicalDeviceFeatures features = { };
    features.wideLines = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;

    std::vector<String> requestedLayers;

    if (m_description.enableValidation)
        requestedLayers.push_back("VK_LAYER_KHRONOS_validation");

    auto enabledLayers = listAndEnableDeviceLayers(requestedLayers);

    std::vector<String> requestedExtensions = { "VK_KHR_swapchain" };

    auto enabledExtensions = listAndEnableDeviceExtensions(requestedExtensions);

    std::vector<VkDeviceQueueCreateInfo> enabledQueues;
    
    assert(m_logfile, getQueue(m_graphicsQueue, enabledQueues, VK_QUEUE_GRAPHICS_BIT, true),
           sprintf(L"The selected device (%S) doesn't support Graphics or Presenation! Please manually select a device!", m_physicalDevice.deviceProperties.deviceName), MRN_DEBUG_INFO);

    bool transferAvailable = getQueue(m_transferQueue, enabledQueues, VK_QUEUE_TRANSFER_BIT, false);

    VkDeviceCreateInfo vdci;
    vdci.sType                                  = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vdci.pNext                                  = nullptr;
    vdci.flags                                  = 0;
    vdci.queueCreateInfoCount                   = static_cast<uint32_t>(enabledQueues.size());
    vdci.pQueueCreateInfos                      = enabledQueues.data();
    vdci.enabledLayerCount                      = static_cast<uint32_t>(enabledLayers.size());
    vdci.ppEnabledLayerNames                    = enabledLayers.data();
    vdci.enabledExtensionCount                  = static_cast<uint32_t>(enabledExtensions.size());
    vdci.ppEnabledExtensionNames                = enabledExtensions.data();
    vdci.pEnabledFeatures                       = &features;

    assert_vulkan(m_logfile, vkCreateDevice(m_physicalDevice.device, &vdci, nullptr, &m_device), L"vkCreateDevice() failed", MRN_DEBUG_INFO);

    m_mainThreadCommandPools.resize(m_physicalDevice.queueFamilyProperties.size(), VK_NULL_HANDLE);

    for (const auto& a : enabledQueues)
    {
        VkCommandPoolCreateInfo vcpci;
        vcpci.sType                             = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        vcpci.pNext                             = nullptr;
        vcpci.flags                             = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vcpci.queueFamilyIndex                  = a.queueFamilyIndex;

        assert_vulkan(m_logfile, vkCreateCommandPool(m_device, &vcpci, nullptr, &m_mainThreadCommandPools[a.queueFamilyIndex]), L"vkCreateCommandPool() failed", MRN_DEBUG_INFO);
    }

    activateQueue(m_graphicsQueue);
    if(transferAvailable)
        activateQueue(m_transferQueue);

    m_logfile->print(WHITE, sprintf(L"Created VkDevice (%.3f ms)", Time::duration(start, Time::now()).getMillisecondsF()));
}


void moraine::GraphicsContext_IVulkan::createVulkanSurface()
{
    Window_IWin32* window = dynamic_cast<Window_IWin32*>(&*m_window);

    VkWin32SurfaceCreateInfoKHR vwsci;
    vwsci.sType                                 = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    vwsci.pNext                                 = nullptr;
    vwsci.flags                                 = 0;
    vwsci.hinstance                             = window->m_instance;
    vwsci.hwnd                                  = window->m_windowHandle;

    assert_vulkan(m_logfile, vkCreateWin32SurfaceKHR(m_instance, &vwsci, nullptr, &m_windowSurface), L"vkCreateWin32SurfaceKHR() failed", MRN_DEBUG_INFO);
}


void moraine::GraphicsContext_IVulkan::createSwapchain()
{
    Time start = Time::now();

    VkSurfaceFormatKHR format = getSurfaceFormat();

    m_swapchainFormat = format.format;
    m_swapchainWidth = m_physicalDevice.surfaceProperites.currentExtent.width;
    m_swapchainHeight = m_physicalDevice.surfaceProperites.currentExtent.height;

    VkSwapchainCreateInfoKHR vsci;
    vsci.sType                                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vsci.pNext                                  = nullptr;
    vsci.flags                                  = 0;
    vsci.surface                                = m_windowSurface;
    vsci.minImageCount                          = moraine::clamp(m_physicalDevice.surfaceProperites.minImageCount, 3u, m_physicalDevice.surfaceProperites.maxImageCount);
    vsci.imageFormat                            = format.format;
    vsci.imageColorSpace                        = format.colorSpace;
    vsci.imageExtent                            = m_physicalDevice.surfaceProperites.currentExtent;
    vsci.imageArrayLayers                       = 1;
    vsci.imageUsage                             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vsci.imageSharingMode                       = VK_SHARING_MODE_EXCLUSIVE;
    vsci.queueFamilyIndexCount                  = 0;
    vsci.pQueueFamilyIndices                    = nullptr;
    vsci.preTransform                           = m_physicalDevice.surfaceProperites.currentTransform;
    vsci.compositeAlpha                         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vsci.presentMode                            = getPresentMode(true);
    vsci.clipped                                = VK_TRUE;
    vsci.oldSwapchain                           = VK_NULL_HANDLE;

    assert_vulkan(m_logfile, vkCreateSwapchainKHR(m_device, &vsci, nullptr, &m_swapchain), L"vkCreateSwapchainKHR() failed", MRN_DEBUG_INFO);

    uint32_t n_swapchainImages;
    assert_vulkan(m_logfile, vkGetSwapchainImagesKHR(m_device, m_swapchain, &n_swapchainImages, nullptr), L"vkGetSwapchainImagesKHR() failed", MRN_DEBUG_INFO);

    m_swapchainImages.resize(n_swapchainImages);
    assert_vulkan(m_logfile, vkGetSwapchainImagesKHR(m_device, m_swapchain, &n_swapchainImages, m_swapchainImages.data()), L"vkGetSwapchainImagesKHR() failed", MRN_DEBUG_INFO);

    m_swapchainImageViews.resize(n_swapchainImages);

    for (size_t i = 0; i < n_swapchainImages; ++i)
    {
        VkImageViewCreateInfo vivci;
        vivci.sType                             = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vivci.pNext                             = nullptr;
        vivci.flags                             = 0;
        vivci.image                             = m_swapchainImages[i];
        vivci.viewType                          = VK_IMAGE_VIEW_TYPE_2D;
        vivci.format                            = format.format;
        vivci.components                        = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        vivci.subresourceRange.aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
        vivci.subresourceRange.layerCount       = 1;
        vivci.subresourceRange.baseArrayLayer   = 0;
        vivci.subresourceRange.levelCount       = 1;
        vivci.subresourceRange.baseMipLevel     = 0;

        assert_vulkan(m_logfile, vkCreateImageView(m_device, &vivci, nullptr, &m_swapchainImageViews[i]), L"vkCreateImageView() failed", MRN_DEBUG_INFO);
    }

    m_logfile->print(WHITE, sprintf(L"Created vulkan swapchain! (%.3f ms)", Time::duration(start, Time::now()).getMillisecondsF()));
}


void moraine::GraphicsContext_IVulkan::createRenderPass()
{
    std::vector<VkAttachmentDescription> attachments(1);

    attachments[0].flags                        = 0;
    attachments[0].format                       = m_swapchainFormat;
    attachments[0].samples                      = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp                = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp               = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout                = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout                  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference reference;
    reference.attachment                        = 0;
    reference.layout                            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::vector<VkSubpassDescription> subpasses(1);

    subpasses[0].flags                          = 0;
    subpasses[0].pipelineBindPoint              = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount           = 0;
    subpasses[0].pInputAttachments              = nullptr;
    subpasses[0].colorAttachmentCount           = 1;
    subpasses[0].pColorAttachments              = &reference;
    subpasses[0].pResolveAttachments            = nullptr;
    subpasses[0].pDepthStencilAttachment        = nullptr;
    subpasses[0].preserveAttachmentCount        = 0;
    subpasses[0].pPreserveAttachments           = nullptr;

    std::vector<VkSubpassDependency> dependencies(1);
    dependencies[0].srcSubpass                  = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass                  = 0;
    dependencies[0].srcStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask               = 0;
    dependencies[0].dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags             = 0;

    VkRenderPassCreateInfo vrpci;
    vrpci.sType                                 = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    vrpci.pNext                                 = nullptr;
    vrpci.flags                                 = 0;
    vrpci.attachmentCount                       = static_cast<uint32_t>(attachments.size());
    vrpci.pAttachments                          = attachments.data();
    vrpci.subpassCount                          = static_cast<uint32_t>(subpasses.size());
    vrpci.pSubpasses                            = subpasses.data();
    vrpci.dependencyCount                       = static_cast<uint32_t>(dependencies.size());
    vrpci.pDependencies                         = dependencies.data();

    assert_vulkan(m_logfile, vkCreateRenderPass(m_device, &vrpci, nullptr, &m_renderPass), L"vkCreateRenderPass() failed", MRN_DEBUG_INFO);
}


void moraine::GraphicsContext_IVulkan::createFrameBuffers()
{
    m_frameBuffers.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_frameBuffers.size(); ++i)
    {
        VkFramebufferCreateInfo vfbci;
        vfbci.sType                              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vfbci.pNext                              = nullptr;
        vfbci.flags                              = 0;
        vfbci.renderPass                         = m_renderPass;  
        vfbci.attachmentCount                    = 1;
        vfbci.pAttachments                       = &m_swapchainImageViews[i];
        vfbci.width                              = m_swapchainWidth;
        vfbci.height                             = m_swapchainHeight;
        vfbci.layers                             = 1;

        assert_vulkan(m_logfile, vkCreateFramebuffer(m_device, &vfbci, nullptr, &m_frameBuffers[i]), L"vkCreateFramebuffer() failed", MRN_DEBUG_INFO);
    }
}


void moraine::GraphicsContext_IVulkan::dispatchTask(Queue queue, std::function<void(VkCommandBuffer)> task)
{
    VkCommandBufferAllocateInfo allocateInfo;
    allocateInfo.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext                          = nullptr;
    allocateInfo.commandPool                    = m_mainThreadCommandPools[queue.queueFamilyIndex];
    allocateInfo.level                          = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount             = 1;

    VkCommandBuffer buffer;
    assert_vulkan(m_logfile, vkAllocateCommandBuffers(m_device, &allocateInfo, &buffer), L"vkAllocateCommandBuffers() failed", MRN_DEBUG_INFO);

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType                             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext                             = nullptr;
    beginInfo.flags                             = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo                  = nullptr;

    assert_vulkan(m_logfile, vkBeginCommandBuffer(buffer, &beginInfo), L"vkBeginCommandBuffer() failed", MRN_DEBUG_INFO);

    task(buffer);

    assert_vulkan(m_logfile, vkEndCommandBuffer(buffer), L"vkEndCommandBuffer() failed", MRN_DEBUG_INFO);

    VkFence fence;

    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType                             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext                             = nullptr;
    fenceInfo.flags                             = 0;

    assert_vulkan(m_logfile, vkCreateFence(m_device, &fenceInfo, nullptr, &fence), L"vkCreateFence() failed", MRN_DEBUG_INFO);

    VkSubmitInfo submitInfo;
    submitInfo.sType                            = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext                            = nullptr;
    submitInfo.waitSemaphoreCount               = 0;
    submitInfo.pWaitSemaphores                  = nullptr;
    submitInfo.pWaitDstStageMask                = nullptr;
    submitInfo.commandBufferCount               = 1;
    submitInfo.pCommandBuffers                  = &buffer;
    submitInfo.signalSemaphoreCount             = 0;
    submitInfo.pSignalSemaphores                = nullptr;

    assert_vulkan(m_logfile, vkQueueSubmit(queue.queue, 1, &submitInfo, fence), L"vkQueueSubmit()", MRN_DEBUG_INFO);

    assert_vulkan(m_logfile, vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX), L"vkWaitForFences()", MRN_DEBUG_INFO);

    vkDestroyFence(m_device, fence, nullptr);
    vkFreeCommandBuffers(m_device, m_mainThreadCommandPools[queue.queueFamilyIndex], 1, &buffer);
}


std::vector<const char*> moraine::GraphicsContext_IVulkan::listAndEnableInstanceLayers(std::vector<String>& requestedLayers)
{
    uint32_t n_layers;
    vkEnumerateInstanceLayerProperties(&n_layers, nullptr);

    std::vector<VkLayerProperties> p_layers(n_layers);
    vkEnumerateInstanceLayerProperties(&n_layers, p_layers.data());

    std::vector<const char*> enabledLayers = { }; // Layers that are both requested and available
    std::vector<bool> availableLayers(requestedLayers.size(), false); // marks which of the requested layers are available

    Table t = createTable(L"Vulkan Instance Layers", WHITE, { L"ID", L"Name", L"Description" });

    for (size_t i = 0; i < p_layers.size(); ++i)
    {
        bool requested = false;

        for (auto a = requestedLayers.begin(); a != requestedLayers.end();++a)
            if (*a == p_layers[i].layerName)
            {
                enabledLayers.push_back(a->mbstr()); // Add to list of enabled layers
                availableLayers[a - requestedLayers.begin()] = true; // Mark layer as available
                requested = true;
                break;
            }

        if(requested)
            t->addRow(GREEN, { moraine::sprintf(L"%d", i), p_layers[i].layerName, p_layers[i].description });
        else if(m_description.enableValidation)
            t->addRow(GREY, { moraine::sprintf(L"%d", i), p_layers[i].layerName, p_layers[i].description });
    }

    for(size_t i = 0; i < requestedLayers.size(); ++i)
        if(availableLayers[i] == false)
            t->addRow(RED, { L"-", requestedLayers[i], L"-" });

    m_logfile->print(t);
    return std::move(enabledLayers);
}


std::vector<const char*> moraine::GraphicsContext_IVulkan::listAndEnableInstanceExtensions(std::vector<String>& requestedExtensions)
{
    uint32_t n_extensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &n_extensions, nullptr);

    std::vector<VkExtensionProperties> p_extensions(n_extensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &n_extensions, p_extensions.data());

    std::vector<const char*> enabledExtensions = { }; // Layers that are both requested and available
    std::vector<bool> availableExtensions(requestedExtensions.size(), false); // marks which of the requested layers are available

    Table t = createTable(L"Vulkan Instance Extensions", WHITE, { L"ID", L"Name" });

    for (size_t i = 0; i < p_extensions.size(); ++i)
    {
        bool requested = false;

        for (auto a = requestedExtensions.begin(); a != requestedExtensions.end();++a)
            if (*a == p_extensions[i].extensionName)
            {
                enabledExtensions.push_back(a->mbstr()); // Add to list of enabled layers
                availableExtensions[a - requestedExtensions.begin()] = true; // Mark layer as available
                requested = true;
                break;
            }

        if (requested)
            t->addRow(GREEN, { moraine::sprintf(L"%d", i), p_extensions[i].extensionName });
        else if (m_description.enableValidation)
            t->addRow(GREY, { moraine::sprintf(L"%d", i), p_extensions[i].extensionName });
    }

    for (size_t i = 0; i < requestedExtensions.size(); ++i)
        if (availableExtensions[i] == false)
            t->addRow(RED, { L"-", requestedExtensions[i] });

    m_logfile->print(t);
    return std::move(enabledExtensions);
}


std::vector<const char*> moraine::GraphicsContext_IVulkan::listAndEnableDeviceLayers(std::vector<String>& requestedLayers)
{
    uint32_t n_layers;
    vkEnumerateDeviceLayerProperties(m_physicalDevice.device, &n_layers, nullptr);

    std::vector<VkLayerProperties> p_layers(n_layers);
    vkEnumerateDeviceLayerProperties(m_physicalDevice.device, &n_layers, p_layers.data());

    std::vector<const char*> enabledLayers = { }; // Layers that are both requested and available
    std::vector<bool> availableLayers(requestedLayers.size(), false); // marks which of the requested layers are available

    Table t = createTable(L"Vulkan Device Layers", WHITE, { L"ID", L"Name", L"Description" });

    for (size_t i = 0; i < p_layers.size(); ++i)
    {
        bool requested = false;

        for (auto a = requestedLayers.begin(); a != requestedLayers.end(); ++a)
            if (*a == p_layers[i].layerName)
            {
                enabledLayers.push_back(a->mbstr()); // Add to list of enabled layers
                availableLayers[a - requestedLayers.begin()] = true; // Mark layer as available
                requested = true;
                break;
            }

        if (requested)
            t->addRow(GREEN, { moraine::sprintf(L"%d", i), p_layers[i].layerName, p_layers[i].description });
        else if (m_description.enableValidation)
            t->addRow(GREY, { moraine::sprintf(L"%d", i), p_layers[i].layerName, p_layers[i].description });
    }

    for (size_t i = 0; i < requestedLayers.size(); ++i)
        if (availableLayers[i] == false)
            t->addRow(RED, { L"-", requestedLayers[i], L"-" });

    m_logfile->print(t);
    return std::move(enabledLayers);
}


std::vector<const char*> moraine::GraphicsContext_IVulkan::listAndEnableDeviceExtensions(std::vector<String>& requestedExtensions)
{
    uint32_t n_extensions;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice.device, nullptr, &n_extensions, nullptr);

    std::vector<VkExtensionProperties> p_extensions(n_extensions);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice.device, nullptr, &n_extensions, p_extensions.data());

    std::vector<const char*> enabledExtensions = { }; // Layers that are both requested and available
    std::vector<bool> availableExtensions(requestedExtensions.size(), false); // marks which of the requested layers are available

    Table t = createTable(L"Vulkan Instance Extensions", WHITE, { L"ID", L"Name" });

    for (size_t i = 0; i < p_extensions.size(); ++i)
    {
        bool requested = false;

        for (auto a = requestedExtensions.begin(); a != requestedExtensions.end(); ++a)
            if (*a == p_extensions[i].extensionName)
            {
                enabledExtensions.push_back(a->mbstr()); // Add to list of enabled layers
                availableExtensions[a - requestedExtensions.begin()] = true; // Mark layer as available
                requested = true;
                break;
            }

        if (requested)
            t->addRow(GREEN, { moraine::sprintf(L"%d", i), p_extensions[i].extensionName });
        else if (m_description.enableValidation)
            t->addRow(GREY, { moraine::sprintf(L"%d", i), p_extensions[i].extensionName });
    }

    for (size_t i = 0; i < requestedExtensions.size(); ++i)
        if (availableExtensions[i] == false)
            t->addRow(RED, { L"-", requestedExtensions[i] });

    m_logfile->print(t);
    return std::move(enabledExtensions);
}


VkSurfaceFormatKHR moraine::GraphicsContext_IVulkan::getSurfaceFormat()
{
    uint32_t n_surfaceFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice.device, m_windowSurface, &n_surfaceFormats, nullptr);

    std::vector<VkSurfaceFormatKHR> p_surfaceFormats(n_surfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice.device, m_windowSurface, &n_surfaceFormats, p_surfaceFormats.data());

    for (const auto& a : p_surfaceFormats)
        if (a.format == VK_FORMAT_B8G8R8A8_UNORM && a.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return a;

    return p_surfaceFormats[0];
}


VkPresentModeKHR moraine::GraphicsContext_IVulkan::getPresentMode(bool useTripleBuffering)
{
    uint32_t n_presentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice.device, m_windowSurface, &n_presentModes, nullptr);

    std::vector<VkPresentModeKHR> p_presentModes(n_presentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice.device, m_windowSurface, &n_presentModes, p_presentModes.data());

    for (const auto& a : p_presentModes)
        if (not useTripleBuffering and a == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        else if (useTripleBuffering and a == VK_PRESENT_MODE_MAILBOX_KHR)
            return VK_PRESENT_MODE_MAILBOX_KHR;

    return VK_PRESENT_MODE_FIFO_KHR;
}


bool moraine::GraphicsContext_IVulkan::getQueue(Queue& out_queue, std::vector<VkDeviceQueueCreateInfo>& out_vdqci, VkQueueFlags flags, bool present)
{
    for (size_t i = 0; i < m_physicalDevice.queueFamilyProperties.size(); ++i) // Loop through all queue families
    {
        if ((flags & ~m_physicalDevice.queueFamilyProperties[i].queueFlags) == 0) // Check if queue family matches flags
        {
            if (present) // Check for presentation support necessary
            {
                VkBool32 supported;
                assert_vulkan(m_logfile, vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice.device, static_cast<uint32_t>(i), m_windowSurface, &supported), 
                              L"vkGetPhysicalDeviceSurfaceSupportKHR() failed", MRN_DEBUG_INFO);

                if (supported == VK_FALSE) // Queue doesn't support presentation
                    continue; // goto next family
            }

            bool noQueueRemaining = false;

            for(auto& a : out_vdqci)
                if (a.queueFamilyIndex == i)
                {
                    if (a.queueCount >= m_physicalDevice.queueFamilyProperties[i].queueCount) // If queue family has been used and no queue is remaining
                    {
                        noQueueRemaining = true;
                        break; // Exit searching list of used queues
                    }

                    out_queue.queueIndex = a.queueCount++;
                    out_queue.queueFamilyIndex = static_cast<uint32_t>(i);

                    return true;
                }

            if (noQueueRemaining)
                continue; // goto next family

            VkDeviceQueueCreateInfo vdqci;
            vdqci.sType                 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vdqci.pNext                 = nullptr;
            vdqci.flags                 = 0;
            vdqci.queueFamilyIndex      = static_cast<uint32_t>(i);
            vdqci.queueCount            = 1; // One queue is created
            vdqci.pQueuePriorities      = s_vulkanQueuePriorities;

            out_vdqci.push_back(std::move(vdqci));

            out_queue.queueIndex = 0; // Always use first queue
            out_queue.queueFamilyIndex = static_cast<uint32_t>(i);

            return true;
        }
    }

    return false;
}


void moraine::GraphicsContext_IVulkan::activateQueue(Queue& queue)
{
    vkGetDeviceQueue(m_device, queue.queueFamilyIndex, queue.queueIndex, &queue.queue);
}


VkBool32 __stdcall moraine::GraphicsContext_IVulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                                   VkDebugUtilsMessageTypeFlagsEXT messageType, 
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        m_logfile->print(RED, moraine::sprintf(L"Vulkan Validation Error %S: %S", pCallbackData->pMessageIdName, pCallbackData->pMessage));
        return VK_FALSE;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        m_logfile->print(YELLOW, moraine::sprintf(L"Vulkan Validation Warning %S: %S", pCallbackData->pMessageIdName, pCallbackData->pMessage));
        return VK_FALSE;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        m_logfile->print(ORANGE, moraine::sprintf(L"Vulkan Validation Vebose Info %S: %S", pCallbackData->pMessageIdName, pCallbackData->pMessage));
        return VK_FALSE;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        m_logfile->print(BLUE, moraine::sprintf(L"Vulkan Validation Info %S: %S", pCallbackData->pMessageIdName, pCallbackData->pMessage));
        return VK_FALSE;
    }

    return VK_FALSE;
}