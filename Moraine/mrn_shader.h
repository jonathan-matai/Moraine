#pragma once

#include "mrn_core.h"
#include "mrn_gfxcontext.h"

namespace moraine
{
    class Shader_T
    {
    public:

        virtual ~Shader_T() = default;
    };

    typedef std::shared_ptr<Shader_T> Shader;

    MRN_API Shader createShader(Stringr shader, GraphicsContext context);
}