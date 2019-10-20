#include "mrn_gfxcontext.h"
#include "mrn_gfxcontext_vk.h"

moraine::GraphicsContext moraine::createGraphicsContext(const GraphicsContextDesc& desc, Logfile logfile, Window window)
{
    return std::make_shared<GraphicsContext_IVulkan>(desc, std::move(logfile), std::move(window));
}
