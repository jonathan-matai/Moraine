#include "mrn_core.h"
#include "mrn_gfxcontext.h"

#include "mrn_gfxcontext_vk.h"

moraine::GraphicsContext moraine::createGraphicsContext(const GraphicsContextDesc& desc, Logfile logfile, Window window)
{
    GraphicsContext context = std::make_shared<GraphicsContext_IVulkan>(desc, std::move(logfile), std::move(window));
    context->m_stagingStack = createStagingStack(context, 10 * 1024 * 1024); // 10MB
    return context;
}