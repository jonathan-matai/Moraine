#include "mrn_renderer_vk.h"

moraine::Renderer_IVulkan::Renderer_IVulkan(GraphicsContext context) :
    m_context(std::static_pointer_cast<GraphicsContext_IVulkan>(context)),
    m_syncObjectIndex(0)
{
    t_shader = createShader(L"C:\\dev\\Moraine\\shader\\shader.json", context);

    std::vector<T_Vertex> t_verticies =
    {
        { float2(-.5f, -.5f), float3(1.0f, 0.0f, 0.0f) },
        { float2(0.5f, -.5f), float3(0.0f, 1.0f, 0.0f) },
        { float2(0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f) },
        { float2(-.5f, 0.5f), float3(1.0f, 0.0f, 1.0f) },
        { float2(0.5f, 0.5f), float3(1.0f, 1.0f, 0.0f) },
    };

    std::vector<uint16_t> t_indicies =
    { 0, 1, 2, 3, 2, 4 };

    t_vertexBuffer = createVertexBuffer(context, sizeof(T_Vertex) * t_verticies.size(), t_verticies.data());
    t_indexBuffer = createIndexBuffer(context, t_indicies.size(), t_indicies.data());

    m_commandBuffers.resize(m_context->m_swapchainImages.size());

    VkCommandBufferAllocateInfo vcbai;
    vcbai.sType                     = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vcbai.pNext                     = nullptr;
    vcbai.commandPool               = m_context->m_mainThreadCommandPools[m_context->m_graphicsQueue.queueFamilyIndex];
    vcbai.level                     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vcbai.commandBufferCount        = static_cast<uint32_t>(m_context->m_swapchainImages.size());

    assert_vulkan(m_context->m_logfile, vkAllocateCommandBuffers(m_context->m_device, &vcbai, m_commandBuffers.data()), L"vkAllocateCommandBuffers() failed", MRN_DEBUG_INFO);

    for (size_t i = 0; i < m_commandBuffers.size(); ++i)
    {
        VkCommandBufferBeginInfo vcbbi;
        vcbbi.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vcbbi.pNext                 = nullptr;
        vcbbi.flags                 = 0;
        vcbbi.pInheritanceInfo      = nullptr;

        assert_vulkan(m_context->m_logfile, vkBeginCommandBuffer(m_commandBuffers[i], &vcbbi), L"vkBeginCommandBuffer() failed", MRN_DEBUG_INFO);

        std::vector<VkClearValue> clearValues(1);
        clearValues[0].color = { 0.0f, 0.0f, 0.2f, 1.0f };

        VkRenderPassBeginInfo vrpbi;
        vrpbi.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vrpbi.pNext                 = nullptr;
        vrpbi.renderPass            = m_context->m_renderPass;
        vrpbi.framebuffer           = m_context->m_frameBuffers[i];
        vrpbi.renderArea.offset     = { 0, 0 };
        vrpbi.renderArea.extent     = { m_context->m_swapchainWidth, m_context->m_swapchainHeight };
        vrpbi.clearValueCount       = static_cast<uint32_t>(clearValues.size());
        vrpbi.pClearValues          = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[i], &vrpbi, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport;
        viewport.x                                      = 0.0f;
        viewport.y                                      = 0.0f;
        viewport.width                                  = static_cast<float>(m_context->m_swapchainWidth);
        viewport.height                                 = static_cast<float>(m_context->m_swapchainHeight);
        viewport.minDepth                               = 0.0f;
        viewport.maxDepth                               = 1.0f;

        vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset                                  = { 0, 0 };
        scissor.extent                                  = { m_context->m_swapchainWidth,m_context->m_swapchainHeight };

        vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, std::static_pointer_cast<Shader_IVulkan>(t_shader)->m_pipeline);

        size_t offset = 0;
        VkBuffer buffer = std::static_pointer_cast<VertexBuffer_IVulkan>(t_vertexBuffer)->getBuffer();
        vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, &buffer, &offset);

        std::static_pointer_cast<IndexBuffer_IVulkan>(t_indexBuffer)->bind(m_commandBuffers[i]);

        vkCmdDrawIndexed(m_commandBuffers[i], 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        assert_vulkan(m_context->m_logfile, vkEndCommandBuffer(m_commandBuffers[i]), L"vkEndCommandBuffer() failed", MRN_DEBUG_INFO);
    }

    m_syncObjects.resize(m_context->m_swapchainImages.size() - 1, SyncObjects(m_context->m_device, m_context->m_logfile));
}

moraine::Renderer_IVulkan::~Renderer_IVulkan()
{
    vkDeviceWaitIdle(m_context->m_device);

    vkFreeCommandBuffers(m_context->m_device, m_context->m_mainThreadCommandPools[m_context->m_graphicsQueue.queueFamilyIndex], static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

void moraine::Renderer_IVulkan::tick(float delta)
{
    assert_vulkan(m_context->m_logfile, vkWaitForFences(m_context->m_device, 1, &m_syncObjects[m_syncObjectIndex].m_fence, VK_TRUE, UINT64_MAX), L"vkWaitForFences() failed", MRN_DEBUG_INFO);
    assert_vulkan(m_context->m_logfile, vkResetFences(m_context->m_device, 1, &m_syncObjects[m_syncObjectIndex].m_fence), L"vkResetFences() failed", MRN_DEBUG_INFO);

    assert_vulkan(m_context->m_logfile, vkAcquireNextImageKHR(m_context->m_device,
                                        m_context->m_swapchain, 
                                        UINT64_MAX, 
                                        m_syncObjects[m_syncObjectIndex].m_renderWaitSemaphore, 
                                        VK_NULL_HANDLE, 
                                        &m_imageIndex),
                  L"vkAcquireNextImageKHR() failed", MRN_DEBUG_INFO);

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

    assert_vulkan(m_context->m_logfile, vkQueueSubmit(m_context->m_graphicsQueue.queue, 1, &submitInfo, m_syncObjects[m_syncObjectIndex].m_fence), L"vkQueueSubmit() failed", MRN_DEBUG_INFO);

    VkPresentInfoKHR vpi;
    vpi.sType                       = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vpi.pNext                       = nullptr;
    vpi.waitSemaphoreCount          = 1;
    vpi.pWaitSemaphores             = &m_syncObjects[m_syncObjectIndex].m_presentWaitSemaphore;
    vpi.swapchainCount              = 1;
    vpi.pSwapchains                 = &m_context->m_swapchain;
    vpi.pImageIndices               = &m_imageIndex;
    vpi.pResults                    = nullptr;

    assert_vulkan(m_context->m_logfile, vkQueuePresentKHR(m_context->m_graphicsQueue.queue, &vpi), L"vkQueuePresentKHR() failed", MRN_DEBUG_INFO);

    m_syncObjectIndex = (m_syncObjectIndex + 1) % m_syncObjects.size();
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
