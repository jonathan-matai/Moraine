#include <moraine.h>

int main()
{
    mrn::ApplicationDesc desc;
    desc.applicationName            = L"Env1 (Moraine)";
    desc.logfilePath                = L"C:\\dev\\Moraine\\log.html";
    desc.graphics.enableValidation  = true;
    desc.graphics.applicationName   = desc.applicationName;
    desc.window.width               = 1600;
    desc.window.height              = 800;
    desc.window.maximized           = false;
    desc.window.minimizable         = true;
    desc.window.resizable           = false;
    desc.window.minimized           = false;
    desc.window.title               = L"Env1 (Moraine)";

    mrn::Application app = mrn::createApplication(desc);

    //mrn::Shader shader = app->createShader(L"C:\\dev\\Moraine\\shader\\dude.json");

    app->run();

    return 0;
}