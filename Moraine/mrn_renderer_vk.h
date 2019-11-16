#pragma once

#include "mrn_renderer.h"
#include "mrn_gfxcontext_vk.h"
#include "mrn_shader_vk.h"
#include "mrn_buffer_vk.h"
#include "mrn_constset_vk.h"
#include "mrn_texture_vk.h"
#include "mrn_object_graphics.h"

namespace moraine
{
    class Renderer_IVulkan : public Renderer_T
    {
    public:

        Renderer_IVulkan(GraphicsContext context, std::list<Layer>* layerStack);
        ~Renderer_IVulkan() override;

        uint32_t tick(float delta) override;

        std::shared_ptr<GraphicsContext_IVulkan> m_context;

        std::vector<VkCommandBuffer> m_commandBuffers;

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

        void recordCommandBuffer(uint32_t frameIndex);

        std::vector<SyncObjects> m_syncObjects;
        uint32_t m_syncObjectIndex;

        uint32_t m_imageIndex;

        struct T_Vertex
        {
            float2 pos;
            float3 color;
        };

        struct T_ImageVertex
        {
            float2 xy;
            float2 uv;
        };

        Shader t_shader;
        VertexBuffer t_vertexBuffer;
        IndexBuffer t_indexBuffer;
        ConstantBuffer t_constantBuffer;
        ConstantSet t_constantSet;
        ConstantArray t_constantArray;
        Texture t_texture;

        std::list<Layer>* m_layerStack;
    };
}