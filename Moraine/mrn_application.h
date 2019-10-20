#pragma once

#include "mrn_core.h"

#include "mrn_window.h"
#include "mrn_gfxcontext.h"
#include "mrn_shader.h"

namespace moraine
{
    struct ApplicationDesc
    {
        String applicationName;
        String logfilePath;
        WindowDesc window;
        GraphicsContextDesc graphics;
    };

    class Application_T
    {
    public:

        virtual ~Application_T() = default;

        virtual void run() = 0;

        virtual Shader createShader(Stringr shader) = 0;
    };

    typedef std::shared_ptr<Application_T> Application;

    MRN_API Application createApplication(const ApplicationDesc& application);
}