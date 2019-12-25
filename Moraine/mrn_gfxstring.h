#pragma once

#include "mrn_font.h"
#include "mrn_math.h"
#include "mrn_object_graphics.h"

namespace moraine
{
    class GraphicsString_T;

    typedef std::shared_ptr<GraphicsString_T> GraphicsString;

    class GraphicsString_T : public Object_Graphics_T
    {
    public:

        MRN_API GraphicsString_T(Stringr text, Font font, size_t fontSize, uint32_t x, uint32_t y, uint32_t reservedChars);
        ~GraphicsString_T();

        bRemove tick(float delta, uint32_t frameIndex) override;

    private:

        Font m_font;
        VertexBuffer m_buffer;
        float2 m_pos;

    };

    MRN_API GraphicsString createGraphicsString(Stringr text, Font font, size_t fontSize, uint32_t x, uint32_t y, uint32_t reservedChars = 0);

}