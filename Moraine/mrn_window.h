#pragma once

#include "mrn_core.h"

namespace moraine
{
    struct WindowDesc
    {
        String      title;
        uint32_t    width;
        uint32_t    height;
        bool        resizable;
        bool        minimized;
        bool        minimizable;
        bool        maximized;
    };

    class Window_T
    {
    public:

        virtual ~Window_T() = default;

        virtual bool tick(float delta) = 0;
    };

    typedef std::shared_ptr<Window_T> Window;

    Window createWindow(const WindowDesc& description, Logfile logfile);
}