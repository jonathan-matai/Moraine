#include "mrn_window_win32.h"

#include <Windows.h>

moraine::Window_IWin32::Window_IWin32(const WindowDesc& description, Logfile logfile) :
    m_instance(GetModuleHandleW(nullptr)),
    m_logfile(logfile),
    m_windowHandle(0),
    m_breakGameLoop(false)
{
    Time start = Time::now();

    WNDCLASSEXW windowClass;
    windowClass.cbSize          = sizeof(WNDCLASSEXW);
    windowClass.style           = 0;
    windowClass.lpfnWndProc     = [](HWND window, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        return reinterpret_cast<Window_IWin32*>(GetWindowLongPtrW(window, GWLP_USERDATA))->windowProcedure(window, message, wParam, lParam);
    };
    windowClass.cbClsExtra      = 0;
    windowClass.cbWndExtra      = sizeof(this);
    windowClass.hInstance       = m_instance;
    windowClass.hIcon           = LoadIconW(m_instance, IDI_APPLICATION);
    windowClass.hCursor         = LoadCursorW(NULL, IDC_ARROW);
    windowClass.hbrBackground   = CreateSolidBrush(RGB(0xff, 0x00, 0x80));
    windowClass.lpszMenuName    = 0;
    windowClass.lpszClassName   = description.title.wcstr();
    windowClass.hIconSm         = 0;

    assert(m_logfile, RegisterClassExW(&windowClass), L"RegisterClassExW() failed!", MRN_DEBUG_INFO);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

    if (description.resizable)
        style |= WS_MAXIMIZEBOX | WS_THICKFRAME;

    if (description.minimizable)
        style |= WS_MINIMIZEBOX;

    if (description.maximized)
        style |= WS_MAXIMIZE;

    if (description.minimized)
        style |= WS_MINIMIZE;

    RECT desktop;
    assert(m_logfile, GetClientRect(GetDesktopWindow(), &desktop), L"GetClientRect(GetDesktopWindow(), ...) failed!", MRN_DEBUG_INFO);

    m_windowHandle = CreateWindowExW(0,
                                     description.title.wcstr(), description.title.wcstr(),
                                     style,
                                     (desktop.right - desktop.left - description.width) / 2,
                                     (desktop.bottom - desktop.top - description.height) / 2,
                                     description.width, description.height,
                                     nullptr,
                                     nullptr, m_instance,
                                     nullptr);

    assert(m_logfile, m_windowHandle, L"CreateWindowExW() failed!", MRN_DEBUG_INFO);

    assert(m_logfile, SetWindowLongPtrW(m_windowHandle, GWLP_USERDATA, reinterpret_cast<long long>(this)) != 0 || GetLastError() == 0, L"SetWindowLongPtrW() failed!", MRN_DEBUG_INFO);

    ShowWindow(m_windowHandle, SW_SHOW);
    assert(m_logfile, UpdateWindow(m_windowHandle), L"UpdateWindow() failed!", MRN_DEBUG_INFO);

    m_logfile->print(WHITE, sprintf(L"Created Window (%.3f ms)", Time::duration(start, Time::now()).getMillisecondsF()));
}

moraine::Window_IWin32::~Window_IWin32()
{
    DestroyWindow(m_windowHandle);
}

LRESULT moraine::Window_IWin32::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:

        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProcW(window, message, wParam, lParam);
}

bool moraine::Window_IWin32::tick(float delta)
{
    MSG message;

    while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
            return false;

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return true;
}
