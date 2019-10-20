#pragma once

#include "mrn_renderer.h"
#include "mrn_gfxcontext_vk.h"
#include "mrn_shader_vk.h"

namespace moraine
{
    class Renderer_IVulkan : public Renderer_T
    {
    public:

        Renderer_IVulkan(GraphicsContext context);
        ~Renderer_IVulkan() override;

        void tick(float delta) override;

        std::shared_ptr<GraphicsContext_IVulkan> m_context;

        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffers;

        void createCommandPool();

        struct SyncObjects
        {
            SyncObjects(VkDevice device, Logfile logfile);
            ~SyncObjects();

            SyncObjects(const SyncObjects&);
            SyncObjects(SyncObjects&&) = delete;
            SyncObjects& operator=(const SyncObjects&) = delete;
            SyncObjects& operator=(SyncObjects&&) = delete;

            mutable VkDevice m_device;
            VkSemaphore m_renderWaitSemaphore;
            VkSemaphore m_presentWaitSemaphore;
            VkFence m_fence;
        };

        std::vector<SyncObjects> m_syncObjects;
        uint32_t m_syncObjectIndex;

        uint32_t m_imageIndex;

        Shader t_shader;
    };
}