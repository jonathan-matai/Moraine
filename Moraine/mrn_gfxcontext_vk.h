#pragma once

#include "mrn_gfxcontext.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <bitset>

#include <vk_mem_alloc.h>

#ifdef _WIN32
#include "mrn_window_win32.h"
#include <vulkan/vulkan_win32.h>
#endif

namespace moraine
{
    inline String vkresult_to_string(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS                                             : return L"VK_SUCCESS";
        case VK_NOT_READY                                           : return L"VK_NOT_READY";
        case VK_TIMEOUT                                             : return L"VK_TIMEOUT";
        case VK_EVENT_SET                                           : return L"VK_EVENT_SET";
        case VK_EVENT_RESET                                         : return L"VK_EVENT_RESET";                                       
        case VK_INCOMPLETE                                          : return L"VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY                            : return L"VK_ERROR_OUT_OF_HOST_MEMORY";                  
        case VK_ERROR_OUT_OF_DEVICE_MEMORY                          : return L"VK_ERROR_OUT_OF_DEVICE_MEMORY";                        
        case VK_ERROR_INITIALIZATION_FAILED                         : return L"VK_ERROR_INITIALIZATION_FAILED";                       
        case VK_ERROR_DEVICE_LOST                                   : return L"VK_ERROR_DEVICE_LOST";                                 
        case VK_ERROR_MEMORY_MAP_FAILED                             : return L"VK_ERROR_MEMORY_MAP_FAILED";                           
        case VK_ERROR_LAYER_NOT_PRESENT                             : return L"VK_ERROR_LAYER_NOT_PRESENT";                           
        case VK_ERROR_EXTENSION_NOT_PRESENT                         : return L"VK_ERROR_EXTENSION_NOT_PRESENT";                       
        case VK_ERROR_FEATURE_NOT_PRESENT                           : return L"VK_ERROR_FEATURE_NOT_PRESENT";                         
        case VK_ERROR_INCOMPATIBLE_DRIVER                           : return L"VK_ERROR_INCOMPATIBLE_DRIVER";                         
        case VK_ERROR_TOO_MANY_OBJECTS                              : return L"VK_ERROR_TOO_MANY_OBJECTS";                            
        case VK_ERROR_FORMAT_NOT_SUPPORTED                          : return L"VK_ERROR_FORMAT_NOT_SUPPORTED";                        
        case VK_ERROR_FRAGMENTED_POOL                               : return L"VK_ERROR_FRAGMENTED_POOL";                             
        case VK_ERROR_OUT_OF_POOL_MEMORY                            : return L"VK_ERROR_OUT_OF_POOL_MEMORY";                          
        case VK_ERROR_INVALID_EXTERNAL_HANDLE                       : return L"VK_ERROR_INVALID_EXTERNAL_HANDLE";                     
        case VK_ERROR_SURFACE_LOST_KHR                              : return L"VK_ERROR_SURFACE_LOST_KHR";                            
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR                      : return L"VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";                    
        case VK_SUBOPTIMAL_KHR                                      : return L"VK_SUBOPTIMAL_KHR";                                    
        case VK_ERROR_OUT_OF_DATE_KHR                               : return L"VK_ERROR_OUT_OF_DATE_KHR";                             
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR                      : return L"VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";                    
        case VK_ERROR_VALIDATION_FAILED_EXT                         : return L"VK_ERROR_VALIDATION_FAILED_EXT";                       
        case VK_ERROR_INVALID_SHADER_NV                             : return L"VK_ERROR_INVALID_SHADER_NV";                           
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT  : return L"VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FRAGMENTATION_EXT                             : return L"VK_ERROR_FRAGMENTATION_EXT";                           
        case VK_ERROR_NOT_PERMITTED_EXT                             : return L"VK_ERROR_NOT_PERMITTED_EXT";                           
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT                    : return L"VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";                  
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT           : return L"VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        default                                                     : return L"[Unknown Vulkan Error]";
        }
    }

    inline void assert_vulkan(Logfile& logfile, VkResult assertion, Stringr message, const DebugInfo& debugInfo)
    {
        if (assertion != VK_SUCCESS)
        {
            logfile->print({ 0xff, 0x00, 0x00 }, sprintf(L"Vulkan Error %s: %s", vkresult_to_string(assertion).wcstr(), message.wcstr()), debugInfo);
            throw std::exception("Vulkan Error!");
        }
    }

    class GraphicsContext_IVulkan : public GraphicsContext_T
    {
    public:

        GraphicsContext_IVulkan(const GraphicsContextDesc& desc, Logfile logfile, Window window);

        ~GraphicsContext_IVulkan() override;

        VkBool32 __stdcall debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

        void addAsyncTask(std::function<void(uint32_t)> perFrameTasks, std::function<void()> finalizationTask);

        void createVulkanInstance();
        void setupValidation();
        void createPhysicalDevice();
        void createLogicalDevice();
        void createVulkanSurface();
        void createSwapchain();
        void createRenderPass();
        void createFrameBuffers();

        struct PhysicalDevice
        {
            VkPhysicalDevice                        device;
            VkPhysicalDeviceProperties              deviceProperties;
            VkPhysicalDeviceFeatures                deviceFeatures;
            VkPhysicalDeviceMemoryProperties        memoryProperties;
            std::vector<VkQueueFamilyProperties>    queueFamilyProperties;
            VkSurfaceCapabilitiesKHR                surfaceProperites;
        };

        struct Queue
        {
            union
            {
                VkQueue queue;
                uint32_t queueIndex;
            };

            uint32_t queueFamilyIndex;
        };

        struct AsyncTask
        {
            std::function<void(uint32_t)> perFrameTasks;
            uint32_t completedFramesBitset;
            std::function<void()> finalizationTask;
        };

        void dispatchTask(Queue queue, std::function<void(VkCommandBuffer)> task);

        std::vector<const char*> listAndEnableInstanceLayers(std::vector<String>& requestedLayers);
        std::vector<const char*> listAndEnableInstanceExtensions(std::vector<String>& requestedExtensions);
        std::vector<const char*> listAndEnableDeviceLayers(std::vector<String>& requestedLayers);
        std::vector<const char*> listAndEnableDeviceExtensions(std::vector<String>& requestedExtensions);
        VkSurfaceFormatKHR       getSurfaceFormat();
        VkPresentModeKHR         getPresentMode(bool useTripleBuffering);

        bool getQueue(Queue& out_queue, std::vector<VkDeviceQueueCreateInfo>& out_vdqci, VkQueueFlags flags, bool present);
        void activateQueue(Queue& queue);
        
        static constexpr float s_vulkanQueuePriorities[16] = { 1.0f };

        GraphicsContextDesc         m_description;
        Logfile                     m_logfile;
        Window                      m_window;
        VkInstance                  m_instance;
        VkDebugUtilsMessengerEXT    m_messenger;
        PhysicalDevice              m_physicalDevice;
        VkDevice                    m_device;
        VkSurfaceKHR                m_windowSurface;
        VkSwapchainKHR              m_swapchain;
        VkFormat                    m_swapchainFormat;
        std::vector<VkImage>        m_swapchainImages;
        std::vector<VkImageView>    m_swapchainImageViews;
        uint32_t                    m_swapchainWidth;
        uint32_t                    m_swapchainHeight;
        std::vector<VkFramebuffer>  m_frameBuffers;
        VkRenderPass                m_renderPass;
        VkImage                     m_colorImage;
        VkImage                     m_depthImage;
        VmaAllocator                m_allocator;
        std::vector<VkCommandPool>  m_mainThreadCommandPools;
        std::list<AsyncTask>        m_asyncTasks;

        bool mt_updateCommandBuffers;

        Queue m_graphicsQueue;
        Queue m_transferQueue;
    };
}