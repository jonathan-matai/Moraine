#include "mrn_core.h"
#include "mrn_gfxstring.h"

moraine::GraphicsString moraine::createGraphicsString(Stringr text, Font font, size_t fontSize, uint32_t x, uint32_t y, uint32_t reservedChars)
{
    return std::make_shared<GraphicsString_T>(text, font, fontSize, x, y, reservedChars);
}

moraine::GraphicsString_T::GraphicsString_T(Stringr text, Font font, size_t fontSize, uint32_t x, uint32_t y, uint32_t reservedChars) :
    Object_Graphics_T(std::make_shared<GraphicsParameters>(font->getShader(), 
                                                           nullptr, 
                                                           std::initializer_list<mrn::VertexBuffer>{  },
                                                           std::initializer_list<mrn::ConstantSet>{ font->getConstantSet() },
                                                           std::initializer_list<mrn::ConstantArray>{ font->getConstantArray() }, 
                                                           std::initializer_list<uint32_t>{ UINT32_MAX }, 
                                                           6u, 
                                                           (uint32_t) text.size())),
    m_font(font),
    m_pos(static_cast<float>(x), static_cast<float>(y))
{
    m_buffer = createVertexBuffer(font->getGraphicsContext(), sizeof(GraphicsStringCharData) * text.size(), nullptr, true, sizeof(GraphicsStringCharData) * max<size_t>(reservedChars, text.size()));
    m_font->createGraphicsStringVertexData(text, static_cast<GraphicsStringCharData*>(m_buffer->data()), fontSize);

    m_graphicsParameters->m_vertexBuffers.push_back(m_buffer);

    *static_cast<GraphicsStringData*>(m_graphicsParameters->m_constantArrays[0]->data(0, m_constantArrayIndicies[0])) =
    {
        m_pos,
        float2(m_font->getGraphicsContext()->getViewportSize())
    };
}

moraine::GraphicsString_T::~GraphicsString_T()
{
}

moraine::bRemove moraine::GraphicsString_T::tick(float delta, uint32_t frameIndex)
{
    

    return false;
}


