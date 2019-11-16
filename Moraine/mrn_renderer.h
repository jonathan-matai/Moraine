#pragma once

#include "mrn_core.h"

#include "mrn_gfxcontext.h"
#include "mrn_layer.h"

namespace moraine
{
    class Renderer_T
    {
    public:

        virtual ~Renderer_T() = default;

        virtual uint32_t tick(float delta) = 0; // return frame index for next frame
    };

    typedef std::shared_ptr<Renderer_T> Renderer;

    MRN_API Renderer createRenderer(GraphicsContext context, std::list<Layer>* layerStack);
}