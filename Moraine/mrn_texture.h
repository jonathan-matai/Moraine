#pragma once

#include "mrn_core.h"
#include "mrn_gfxcontext.h"
#include "mrn_constset.h"

namespace moraine
{
    class Texture_T : public ConstantResource_T
    {
    public:

        Texture_T() :
            ConstantResource_T(CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER)
        { }

        virtual ~Texture_T() = default;
    };

    typedef std::shared_ptr<Texture_T> Texture;

    MRN_API Texture createTexture(GraphicsContext context, Stringr imagePath);
}