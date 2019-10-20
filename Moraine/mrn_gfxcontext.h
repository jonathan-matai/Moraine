#pragma once

#include "mrn_core.h"
#include "mrn_window.h"

namespace moraine
{
    struct GraphicsContextDesc
    {
        String  applicationName;
        bool    enableValidation;
    };

    class GraphicsContext_T
    {
    public:

        virtual ~GraphicsContext_T() = default;
    };

    typedef std::shared_ptr<GraphicsContext_T> GraphicsContext;

    MRN_API GraphicsContext createGraphicsContext(const GraphicsContextDesc& desc, Logfile logfile, Window window);
}