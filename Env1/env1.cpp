#include <moraine.h>
#include "Spiral.h"

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

    mrn::Shader shader = app->createShader(L"C:\\dev\\Moraine\\Env1\\spiral\\spiral.json");

    mrn::Texture texture = app->createTexture(L"C:\\dev\\Moraine\\Env1\\spiral.png");

    struct Vertex
    {
        mrn::float2 pos;
        mrn::float2 tex;
    };

    std::array<Vertex, 6> verticies { {
        { { -0.3f, -0.3f }, { 0.0f, 0.0f } },
        { {  0.3f, -0.3f }, { 1.0f, 0.0f } },
        { {  0.3f,  0.3f }, { 1.0f, 1.0f } },
        { {  0.3f,  0.3f }, { 1.0f, 1.0f } },
        { { -0.3f,  0.3f }, { 0.0f, 1.0f } },
        { { -0.3f, -0.3f }, { 0.0f, 0.0f } }
    } };

    mrn::VertexBuffer vertexBuffer = app->createVertexBuffer(verticies.size() * sizeof(Vertex), verticies.data(), false, 0);

    std::array<uint16_t, 6> indicies { 0, 1, 2, 2, 3, 0 };

    mrn::IndexBuffer indexBuffer = app->createIndexBuffer(indicies.size(), indicies.data());

    mrn::ConstantArray constantArray = app->createConstantArray(sizeof(Spiral::ConstantBufferData), 10, true);

    mrn::ConstantSet constantSet = mrn::createConstantSet(shader, 0, { { texture, 0 }, { constantArray, 1 } });

    Spiral::s_graphicsParameters = std::make_shared<mrn::GraphicsParameters>(shader, 
                                                                             nullptr,//indexBuffer, 
                                                                             std::initializer_list<mrn::VertexBuffer>{ vertexBuffer }, 
                                                                             std::initializer_list<mrn::ConstantSet>{ constantSet }, 
                                                                             std::initializer_list<mrn::ConstantArray>{ constantArray },
                                                                             std::initializer_list<uint32_t>{ UINT32_MAX },
                                                                             6,
                                                                             1);

    mrn::Font font = app->createFont(L"C:\\Users\\zehre\\AppData\\Local\\Microsoft\\Windows\\Fonts\\CascadiaCode-Regular-VTT.ttf", 300);

    //mrn::GraphicsString str = mrn::createGraphicsString(L"Hello World", font, 100, 50, 50);

    mrn::Layer layer = mrn::createLayer(L"Spiral Layer");


    layer->add(std::make_unique<Spiral>(mrn::float2(-0.5f, -0.5f), mrn::RED));
    layer->add(std::make_unique<Spiral>(mrn::float2(+0.5f, -0.5f), mrn::BLUE));
    layer->add(std::make_unique<Spiral>(mrn::float2(+0.5f, +0.5f), mrn::GREEN));
    layer->add(std::make_unique<Spiral>(mrn::float2(-0.5f, +0.5f), mrn::ORANGE));

    layer->add(std::make_unique<mrn::GraphicsString_T>(L"Hello World", font, 100, 50, 50, 0));

    app->addLayer(layer);

    app->t_updateCommandBuffers();

    app->run();

    return 0;
}

std::shared_ptr<mrn::GraphicsParameters> Spiral::s_graphicsParameters = { };