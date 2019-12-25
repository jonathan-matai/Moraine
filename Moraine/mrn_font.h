#pragma once

#include "mrn_gfxcontext.h"

namespace moraine
{
    MRN_DECLARE_HANDLE(Texture)

    struct GraphicsStringCharData
    {
        RectangleU  spriteSheetLocation;
        float       xOffset;
        float       yOffset;
        float       size;
        Color       color;
    };

    struct GraphicsStringData
    {
        float2 pos;
        float2 viewportSize;
    };

    class Font_T
    {
    public:

        virtual ~Font_T() = default;

        virtual void createGraphicsStringVertexData(Stringr text, GraphicsStringCharData* out_vertexData, size_t fontSize) const = 0;

        virtual void t_getTextureShader(Shader* outShader, Texture* outTexture) const = 0;

        GraphicsContext getGraphicsContext() const { return m_context; }
        Shader getShader() const { return m_fontShader; }
        ConstantSet getConstantSet() const { return m_constantSet; }
        ConstantArray getConstantArray() const { return m_array; }

    protected:

        GraphicsContext m_context;
        Shader          m_fontShader;
        ConstantSet     m_constantSet;
        ConstantArray   m_array;
        uint32_t        m_maxFontSize;
    };

    typedef std::shared_ptr<Font_T> Font;

    MRN_API Font createFont(GraphicsContext context, Stringr ttfFile, uint32_t maxPixelHeight);
}