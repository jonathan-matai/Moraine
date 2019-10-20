#include "mrn_window.h"
#include "mrn_window_win32.h"

moraine::Window moraine::createWindow(const WindowDesc& description, Logfile logfile)
{
    return std::make_shared<Window_IWin32>(description, logfile);
}
