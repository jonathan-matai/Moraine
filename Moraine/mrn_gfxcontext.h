#pragma once

#include "mrn_window.h"
#include "mrn_buffer.h"

namespace moraine
{
    class StagingStack_T;
    typedef std::shared_ptr<StagingStack_T> StagingStack;

    struct GraphicsContextDesc
    {
        String  applicationName;
        bool    enableValidation;
    };

    class GraphicsContext_T
    {
    public:

        GraphicsContext_T(Logfile logfile) :
            m_logfile(logfile),
            m_viewportWidth(0),
            m_viewportHeight(0)
        { }

        virtual ~GraphicsContext_T() = default;

        Logfile getLogfile() const { return m_logfile; };
        float2 getViewportSize() const { return float2(static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight)); }

        StagingStack m_stagingStack;
        uint32_t m_viewportWidth, m_viewportHeight;

    protected:

        Logfile m_logfile;
    };

    typedef std::shared_ptr<GraphicsContext_T> GraphicsContext;

    MRN_API GraphicsContext createGraphicsContext(const GraphicsContextDesc& desc, Logfile logfile, Window window);
}