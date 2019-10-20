#pragma once

#include "mrn_window.h"
#include <Windows.h>

namespace moraine
{
    class Window_IWin32 : public Window_T
    {
    public:

        Window_IWin32(const WindowDesc& description, Logfile logfile);
        ~Window_IWin32() override;

        LRESULT windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

        bool tick(float delta) override;

        HINSTANCE m_instance;
        HWND m_windowHandle;
        Logfile m_logfile;
        bool m_breakGameLoop;
    };
}