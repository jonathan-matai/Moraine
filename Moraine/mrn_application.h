#pragma once

#include "mrn_window.h"
#include "mrn_gfxcontext.h"
#include "mrn_shader.h"
#include "mrn_texture.h"
#include "mrn_buffer.h"
#include "mrn_layer.h"
#include "mrn_gfxstring.h"

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
        virtual Texture createTexture(Stringr texture) = 0;
        virtual VertexBuffer createVertexBuffer(size_t size, void* data, bool frequentUpdate, size_t reservedSize) = 0;
        virtual IndexBuffer createIndexBuffer(size_t indexCount, uint16_t* indexData) = 0;
        virtual IndexBuffer createIndexBuffer(size_t indexCount, uint32_t* indexData) = 0;
        virtual ConstantBuffer createConstantBuffer(size_t size, bool updateEveryFrame) = 0;
        virtual ConstantArray createConstantArray(size_t elementSize, uint32_t initialElementCount, bool updateEveryFrame) = 0;
        virtual Font createFont(Stringr ttfFile, uint32_t maxPixelHeight) = 0;

        virtual void addLayer(Layer layer) = 0;

        virtual void t_updateCommandBuffers() = 0;
    };

    typedef std::shared_ptr<Application_T> Application;

    MRN_API Application createApplication(const ApplicationDesc& application);
}