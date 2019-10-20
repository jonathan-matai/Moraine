#include "mrn_application.h"

#include "mrn_renderer.h"

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
            m_renderer = createRenderer(m_gfxContext);
        }

        ~Application_I() override
        {

        }

        void run() override
        {
            while (m_window->tick(0.0f))
            {
                m_renderer->tick(0.0f);
            }
        }

        Shader createShader(Stringr shader) override
        {
            return moraine::createShader(shader, m_gfxContext);
        }

        Logfile m_logfile;
        Window m_window;
        GraphicsContext m_gfxContext;
        Renderer m_renderer;
    };
}

moraine::Application moraine::createApplication(const ApplicationDesc& application)
{
    return std::make_shared<Application_I>(application);
}
