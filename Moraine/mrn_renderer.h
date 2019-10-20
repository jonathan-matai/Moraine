#pragma once

#include "mrn_core.h"

#include "mrn_gfxcontext.h"

namespace moraine
{
    class Renderer_T
    {
    public:

        virtual ~Renderer_T() = default;

        virtual void tick(float delta) = 0;
    };

    typedef std::shared_ptr<Renderer_T> Renderer;

    MRN_API Renderer createRenderer(GraphicsContext context);
}