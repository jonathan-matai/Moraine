#include "mrn_application.h"

#include "mrn_renderer.h"

// temp
#include "mrn_gfxcontext_vk.h"

namespace moraine
{
    class Application_I : public Application_T
    {
    public:

        Application_I(const ApplicationDesc& desc)
        {
            m_logfile = createLogfile(desc.logfilePath, desc.applicationName);
            m_window = createWindow(desc.window, m_logfile);
            m_gfxContext = createGraphicsContext(desc.graphics, m_logfile, m_window);
            m_renderer = createRenderer(m_gfxContext, &m_layerStack);
        }

        ~Application_I() override
        {

        }

        void run() override
        {
            Time lastTime = Time::now();
            float delta = 0.0f;

            while (m_window->tick(0.0f))
            {
                Time currentTime = Time::now();
                delta = Time::duration(lastTime, currentTime).getMillisecondsF();
                lastTime = currentTime;
                uint32_t frameIndex = m_renderer->tick(delta);

                for (const auto& a : m_layerStack)
                    a->tick(delta, frameIndex);
            }
        }

        Shader createShader(Stringr shader) override
        {
            return moraine::createShader(shader, m_gfxContext);
        }

        Texture createTexture(Stringr texture) override
        {
            return moraine::createTexture(m_gfxContext, texture);
        }

        VertexBuffer createVertexBuffer(size_t size, void* data) override
        {
            return moraine::createVertexBuffer(m_gfxContext, size, data);
        }

        IndexBuffer createIndexBuffer(size_t indexCount, uint16_t* indexData) override
        {
            return moraine::createIndexBuffer(m_gfxContext, indexCount, indexData);
        }

        IndexBuffer createIndexBuffer(size_t indexCount, uint32_t* indexData) override
        {
            return moraine::createIndexBuffer(m_gfxContext, indexCount, indexData);
        }

        ConstantBuffer createConstantBuffer(size_t size, bool updateEveryFrame) override
        {
            return moraine::createConstantBuffer(m_gfxContext, size, updateEveryFrame);
        }

        ConstantArray createConstantArray(size_t elementSize, size_t initialElementCount, bool updateEveryFrame) override
        {
            return moraine::createConstantArray(m_gfxContext, elementSize, initialElementCount, updateEveryFrame);
        }

        void addLayer(Layer layer) override
        {
            m_layerStack.push_back(layer);
        }

        void t_updateCommandBuffers() override
        {
            std::static_pointer_cast<GraphicsContext_IVulkan>(m_gfxContext)->addAsyncTask(nullptr, nullptr);
        }


        Logfile m_logfile;
        Window m_window;
        GraphicsContext m_gfxContext;
        Renderer m_renderer;

        std::list<Layer> m_layerStack;
    };
}

moraine::Application moraine::createApplication(const ApplicationDesc& application)
{
    return std::make_shared<Application_I>(application);
}
