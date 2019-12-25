#include "mrn_core.h"
#include "mrn_renderer_vk.h"

// temp
#include <stb_image.h>

moraine::Renderer_IVulkan::Renderer_IVulkan(GraphicsContext context, std::list<Layer>* layerStack) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_syncObjectIndex(0),
    m_layerStack(layerStack)
{
    t_shader = createShader(L"C:\\dev\\Moraine\\shader\\sweden.json", context);

    std::vector<T_ImageVertex> t_verticies =
    {
        { { -1.0f, -1.0f }, { 0.0f, 0.0f } },
        { { +1.0f, -1.0f }, { 1.0f, 0.0f } },
        { { +1.0f, +1.0f }, { 1.0f, 1.0f } },
        { { +1.0f, +1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, +1.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f }, { 0.0f, 0.0f } },

        { { -0.5f, -1.0f }, { 0.0f, 0.0f } },
        { { +0.5f, -1.0f }, { 1.0f, 0.0f } },
        { { +0.5f, +1.0f }, { 1.0f, 1.0f } },
        { { +0.5f, +1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, +1.0f }, { 0.0f, 1.0f } },
        { { -0.5f, -1.0f }, { 0.0f, 0.0f } }
    };

    t_vertexBuffer = createVertexBuffer(context, sizeof(T_ImageVertex) * t_verticies.size(), t_verticies.data(), false);

    //t_atlas = createTextureAtlas(m_context, IMAGE_COLOR_CHANNLES_RGB_ALPHA, 1024);

    t_texture = createTexture(context, L"C:\\dev\\Moraine\\shader\\sweden.png");

    t_constantSet = createConstantSet(t_shader, 0, { { t_texture, 0 } });

    //int width, height, channelCount;
    //
    //void* data = stbi_load("C:\\Users\\zehre\\Pictures\\i1.png", &width, &height, &channelCount, STBI_rgb_alpha);
    //memcpy_s(t_atlas->allocateImageSpace((uint32_t) width, (uint32_t) height), width * height * STBI_rgb_alpha, data, width * height * STBI_rgb_alpha);
    //
    //data = stbi_load("C:\\Users\\zehre\\Pictures\\i2.png", &width, &height, &channelCount, STBI_rgb_alpha);
    //memcpy_s(t_atlas->allocateImageSpace((uint32_t) width, (uint32_t) height), width * height * STBI_rgb_alpha, data, width * height * STBI_rgb_alpha);
    //
    //data = stbi_load("C:\\Users\\zehre\\Pictures\\i3.jpg", &width, &height, &channelCount, STBI_rgb_alpha);
    //memcpy_s(t_atlas->allocateImageSpace((uint32_t) width, (uint32_t) height), width * height * STBI_rgb_alpha, data, width * height * STBI_rgb_alpha);
    //
    //data = stbi_load("C:\\Users\\zehre\\Pictures\\i4.jpg", &width, &height, &channelCount, STBI_rgb_alpha);
    //memcpy_s(t_atlas->allocateImageSpace((uint32_t) width, (uint32_t) height), width * height * STBI_rgb_alpha, data, width * height * STBI_rgb_alpha);
    //
    //t_atlas->addImageSpacesToAtlas();

    //t_font = createFont(context, L"C:\\Users\\zehre\\AppData\\Local\\Microsoft\\Windows\\Fonts\\CascadiaCode-Regular-VTT.ttf", 300);

    //Texture t;

    //t_font->t_getTextureShader(&t_fontShader, &t);

    //t_constantSet2 = createConstantSet(t_fontShader, 0, { { t, 0 } });

    m_commandBuffers.resize(m_context->m_swapchainImages.size());

    VkCommandBufferAllocateInfo vcbai;
    vcbai.sType                     = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vcbai.pNext                     = nullptr;
    vcbai.commandPool               = m_context->m_mainThreadCommandPools[m_context->m_graphicsQueue.queueFamilyIndex];
    vcbai.level                     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vcbai.commandBufferCount        = static_cast<uint32_t>(m_context->m_swapchainImages.size());

    assert_vulkan(m_context->getLogfile(), vkAllocateCommandBuffers(m_context->m_device, &vcbai, m_commandBuffers.data()), L"vkAllocateCommandBuffers() failed", MRN_DEBUG_INFO);

    for (uint32_t i = 0; i < m_commandBuffers.size(); ++i)
        recordCommandBuffer(i);

    m_syncObjects.resize(m_context->m_swapchainImages.size() - 1, SyncObjects(m_context->m_device, m_context->getLogfile()));

    assert_vulkan(m_context->getLogfile(), vkAcquireNextImageKHR(m_context->m_device,
                  m_context->m_swapchain,
                  UINT64_MAX,
                  m_syncObjects[m_syncObjectIndex].m_renderWaitSemaphore,
                  VK_NULL_HANDLE,
                  &m_imageIndex),
                  L"vkAcquireNextImageKHR() failed", MRN_DEBUG_INFO);
}

moraine::Renderer_IVulkan::~Renderer_IVulkan()
{
    vkDeviceWaitIdle(m_context->m_device);

    vkFreeCommandBuffers(m_context->m_device, m_context->m_mainThreadCommandPools[m_context->m_graphicsQueue.queueFamilyIndex], static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

uint32_t moraine::Renderer_IVulkan::tick(float delta)
{
    assert_vulkan(m_context->getLogfile(), vkWaitForFences(m_context->m_device, 1, &m_syncObjects[m_syncObjectIndex].m_fence, VK_TRUE, UINT64_MAX), L"vkWaitForFences() failed", MRN_DEBUG_INFO);
    assert_vulkan(m_context->getLogfile(), vkResetFences(m_context->m_device, 1, &m_syncObjects[m_syncObjectIndex].m_fence), L"vkResetFences() failed", MRN_DEBUG_INFO);

    if (not m_context->m_asyncTasks.empty())
    {
        for (auto a = m_context->m_asyncTasks.begin(); a != m_context->m_asyncTasks.end();)
            if (a->completedFramesBitset & 1 << m_imageIndex) // frame not dispatched
            {
                if(a->perFrameTasks)
                    a->perFrameTasks(m_imageIndex);

                a->completedFramesBitset ^= 1 << m_imageIndex; // mark frame as dispatched

                if (a->completedFramesBitset == 0) // all frames dispatched
                {
                    if(a->finalizationTask)
                        a->finalizationTask();
                    m_context->m_asyncTasks.erase(a++); // erase then increment
                }
                else
                    ++a; // increment
            }
            else
                ++a; // increment

        recordCommandBuffer(m_imageIndex);
    }

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo;
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext                = nullptr;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &m_syncObjects[m_syncObjectIndex].m_renderWaitSemaphore;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &m_commandBuffers[m_imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_syncObjects[m_syncObjectIndex].m_presentWaitSemaphore;

    assert_vulkan(m_context->getLogfile(), vkQueueSubmit(m_context->m_graphicsQueue.queue, 1, &submitInfo, m_syncObjects[m_syncObjectIndex].m_fence), L"vkQueueSubmit() failed", MRN_DEBUG_INFO);

    VkPresentInfoKHR vpi;
    vpi.sType                       = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vpi.pNext                       = nullptr;
    vpi.waitSemaphoreCount          = 1;
    vpi.pWaitSemaphores             = &m_syncObjects[m_syncObjectIndex].m_presentWaitSemaphore;
    vpi.swapchainCount              = 1;
    vpi.pSwapchains                 = &m_context->m_swapchain;
    vpi.pImageIndices               = &m_imageIndex;
    vpi.pResults                    = nullptr;

    assert_vulkan(m_context->getLogfile(), vkQueuePresentKHR(m_context->m_graphicsQueue.queue, &vpi), L"vkQueuePresentKHR() failed", MRN_DEBUG_INFO);

    m_syncObjectIndex = (m_syncObjectIndex + 1) % m_syncObjects.size();

    assert_vulkan(m_context->getLogfile(), vkAcquireNextImageKHR(m_context->m_device,
                  m_context->m_swapchain,
                  UINT64_MAX,
                  m_syncObjects[m_syncObjectIndex].m_renderWaitSemaphore,
                  VK_NULL_HANDLE,
                  &m_imageIndex),
                  L"vkAcquireNextImageKHR() failed", MRN_DEBUG_INFO);

    return m_imageIndex;
}

void moraine::Renderer_IVulkan::recordCommandBuffer(uint32_t i)
{
    VkCommandBufferBeginInfo vcbbi;
    vcbbi.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vcbbi.pNext                 = nullptr;
    vcbbi.flags                 = 0;
    vcbbi.pInheritanceInfo      = nullptr;

    assert_vulkan(m_context->getLogfile(), vkBeginCommandBuffer(m_commandBuffers[i], &vcbbi), L"vkBeginCommandBuffer() failed", MRN_DEBUG_INFO);

    std::vector<VkClearValue> clearValues(1);
    clearValues[0].color = { 0.0f, 0.0f, 0.2f, 1.0f };

    VkRenderPassBeginInfo vrpbi;
    vrpbi.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vrpbi.pNext                 = nullptr;
    vrpbi.renderPass            = m_context->m_renderPass;
    vrpbi.framebuffer           = m_context->m_frameBuffers[i];
    vrpbi.renderArea.offset     = { 0, 0 };
    vrpbi.renderArea.extent     = { m_context->m_viewportWidth, m_context->m_viewportHeight };
    vrpbi.clearValueCount       = static_cast<uint32_t>(clearValues.size());
    vrpbi.pClearValues          = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffers[i], &vrpbi, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.x                  = 0.0f;
    viewport.y                  = 0.0f;
    viewport.width              = static_cast<float>(m_context->m_viewportWidth);
    viewport.height             = static_cast<float>(m_context->m_viewportHeight);
    viewport.minDepth           = 0.0f;
    viewport.maxDepth           = 1.0f;

    vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset              = { 0, 0 };
    scissor.extent              = { m_context->m_viewportWidth,m_context->m_viewportHeight };

    vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

    std::static_pointer_cast<Shader_IVulkan>(t_shader)->bind(m_commandBuffers[i]);
    std::static_pointer_cast<VertexBuffer_IVulkan>(t_vertexBuffer)->bind(m_commandBuffers[i], 0, 0);
    std::static_pointer_cast<ConstantSet_IVulkan>(t_constantSet)->bind(m_commandBuffers[i], static_cast<uint32_t>(i), { });
    vkCmdDraw(m_commandBuffers[i], 6, 1, 0, 0);

    //std::static_pointer_cast<Shader_IVulkan>(t_fontShader)->bind(m_commandBuffers[i]);
    //std::static_pointer_cast<VertexBuffer_IVulkan>(t_vertexBuffer)->bind(m_commandBuffers[i], 0, sizeof(T_ImageVertex) * 6);
    //std::static_pointer_cast<ConstantSet_IVulkan>(t_constantSet2)->bind(m_commandBuffers[i], static_cast<uint32_t>(i), { });
    //vkCmdDraw(m_commandBuffers[i], 6, 1, 0, 0);


    for (auto& a : *m_layerStack)
    {
        for(auto& b : *a)
            if (b->type() & OBJECT_TYPE_GRAPHICS)
            {
                auto obj = static_cast<Object_Graphics_T*>(&*b);
                
                std::static_pointer_cast<Shader_IVulkan>(obj->m_graphicsParameters->m_shader)->bind(m_commandBuffers[i]);

                for (const auto& c : obj->m_graphicsParameters->m_vertexBuffers)
                        std::static_pointer_cast<VertexBuffer_IVulkan>(c)->bind(m_commandBuffers[i], 0, 0);

                if(static_cast<bool>(obj->m_graphicsParameters->m_indexBuffer))
                    std::static_pointer_cast<IndexBuffer_IVulkan>(obj->m_graphicsParameters->m_indexBuffer)->bind(m_commandBuffers[i]);

                for (const auto& c : obj->m_graphicsParameters->m_constantSets)
                    std::static_pointer_cast<ConstantSet_IVulkan>(c)->bind(m_commandBuffers[i], static_cast<uint32_t>(i), obj->m_constantArrayIndicies);

                if (static_cast<bool>(obj->m_graphicsParameters->m_indexBuffer))
                    vkCmdDrawIndexed(m_commandBuffers[i],
                                     obj->m_graphicsParameters->m_vertexCount,
                                     obj->m_graphicsParameters->m_instanceCount > 1 ? obj->m_graphicsParameters->m_instanceCount : 1,
                                     0,
                                     0,
                                     0);
                else
                    vkCmdDraw(m_commandBuffers[i],
                              obj->m_graphicsParameters->m_vertexCount,
                              obj->m_graphicsParameters->m_instanceCount > 1 ? obj->m_graphicsParameters->m_instanceCount : 1,
                              0,
                              0);
            }
    }

    vkCmdEndRenderPass(m_commandBuffers[i]);

    assert_vulkan(m_context->getLogfile(), vkEndCommandBuffer(m_commandBuffers[i]), L"vkEndCommandBuffer() failed", MRN_DEBUG_INFO);
}

moraine::Renderer_IVulkan::SyncObjects::SyncObjects(VkDevice device, Logfile logfile) :
    m_device(device)
{
    VkSemaphoreCreateInfo vsci;
    vsci.sType                      = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vsci.pNext                      = nullptr;
    vsci.flags                      = 0;

    assert_vulkan(logfile, vkCreateSemaphore(device, &vsci, nullptr, &m_renderWaitSemaphore), L"vkCreateSemaphore() failed", MRN_DEBUG_INFO);
    assert_vulkan(logfile, vkCreateSemaphore(device, &vsci, nullptr, &m_presentWaitSemaphore), L"vkCreateSemaphore() failed", MRN_DEBUG_INFO);

    VkFenceCreateInfo vfci;
    vfci.sType                      = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vfci.pNext                      = nullptr;
    vfci.flags                      = VK_FENCE_CREATE_SIGNALED_BIT;

    assert_vulkan(logfile, vkCreateFence(device, &vfci, nullptr, &m_fence), L"vkCreateFence() failed", MRN_DEBUG_INFO);
}

moraine::Renderer_IVulkan::SyncObjects::~SyncObjects()
{
    if (not m_device)
        return;

    vkDestroySemaphore(m_device, m_renderWaitSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_presentWaitSemaphore, nullptr);
    vkDestroyFence(m_device, m_fence, nullptr);
}

moraine::Renderer_IVulkan::SyncObjects::SyncObjects(const SyncObjects& o)
{
    m_device = o.m_device;
    m_renderWaitSemaphore = o.m_renderWaitSemaphore;
    m_presentWaitSemaphore = o.m_presentWaitSemaphore;
    m_fence = o.m_fence;
    o.m_device = nullptr;
}
