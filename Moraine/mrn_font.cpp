#include "mrn_core.h"
#include "mrn_font.h"

#include <stb_truetype.h>

#include "mrn_shader.h"
#include "mrn_constset.h"
#include "mrn_texture.h"

namespace moraine
{
    class Font_I : public Font_T
    {
    public:

        Font_I(GraphicsContext context, Stringr ttfFile, uint32_t maxPixelHeight);
        ~Font_I();

        void createGraphicsStringVertexData(Stringr text, GraphicsStringCharData* out_vertexData, size_t fontSize) const override;

        void t_getTextureShader(Shader* outShader, Texture* outTexture) const override
        {
            *outShader = m_fontShader;
            *outTexture = m_atlas->getTexture();
        }

    protected:

        struct FontChar
        {
            RectangleU  m_spriteSheetLocation;
            int32_t     m_offsetX;
            int32_t     m_offsetY;
            uint32_t    m_width;
        };

        FontChar findOrAllocChar(wchar_t character) const;

        //void allocUndefinedChars(Stringr chars);

        constexpr const static wchar_t* s_fontShaderPath = L"C:\\dev\\Moraine\\Env1\\res\\shaders\\font\\shader.json";
        static std::weak_ptr<Shader_T> s_fontShader;

        Allocation      m_ttfFile;
        stbtt_fontinfo  m_fontInfo;
        float           m_stbFontSize;

        int32_t         m_ascent; // unscaled
        int32_t         m_descent; // unscaled
        int32_t         m_lineGap; // unscaled

        TextureAtlas    m_atlas;

        mutable std::unordered_map<wchar_t, FontChar> m_charData;
    };

    
}

moraine::Font_I::Font_I(GraphicsContext context, Stringr ttfPath, uint32_t maxPixelHeight)
{
    Time start = Time::now();

    m_context = context;
    m_maxFontSize = maxPixelHeight;

    m_ttfFile = loadFile(context->getLogfile(), ttfPath);

    assert(m_context->getLogfile(), stbtt_InitFont(&m_fontInfo, m_ttfFile.allocation.get(), 0) != 0, sprintf(L"Reading TTF Metadata from font \"%s failed!\"", ttfPath.wcstr()), MRN_DEBUG_INFO);

    m_stbFontSize = stbtt_ScaleForPixelHeight(&m_fontInfo, static_cast<float>(maxPixelHeight));

    stbtt_GetFontVMetrics(&m_fontInfo, &m_ascent, &m_descent, &m_lineGap);

    if (s_fontShader.expired())
    {
        m_fontShader = createShader(s_fontShaderPath, context);
        s_fontShader = m_fontShader;
    }
    else
        m_fontShader = s_fontShader.lock();

    m_atlas = createTextureAtlas(context, IMAGE_COLOR_CHANNELS_BW, 2048);

    m_array = createConstantArray(context, sizeof(GraphicsStringData), 10, false);

    m_constantSet = createConstantSet(m_fontShader, 0,
                                      {
                                          { m_atlas->getTexture(), 0 },
                                          { m_array, 1 }
                                      });

    String preallocatedChars = L"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZzƒ‰÷ˆ‹‹ﬂ123456789()[]{}<>\"\'\\/|,;.:-_#~^∞Ä@!ß$%&=?¥`";

    for (const wchar_t* i = &preallocatedChars.wcstr()[0]; i != &preallocatedChars.wcstr()[preallocatedChars.length()]; ++i)
        findOrAllocChar(*i);

    m_atlas->addImageSpacesToAtlas();
}

moraine::Font_I::~Font_I()
{
}

void moraine::Font_I::createGraphicsStringVertexData(Stringr text, GraphicsStringCharData* out_vertexData, size_t fontSize) const
{
    float scaleFactor = stbtt_ScaleForPixelHeight(&m_fontInfo, static_cast<float>(fontSize));

    uint32_t xOffset = 0;
    uint32_t yOffset = static_cast<uint32_t>(m_ascent * scaleFactor);

    for (size_t i = 0; i < text.length(); ++i)
    {
        FontChar data = findOrAllocChar(text.wcstr()[i]);

        int kerning = stbtt_GetCodepointKernAdvance(&m_fontInfo, text.wcstr()[i], text.wcstr()[i + 1]);

        out_vertexData[i] =
        {
            data.m_spriteSheetLocation,
            static_cast<float>(xOffset) + static_cast<float>(data.m_offsetX) * fontSize / m_maxFontSize,
            static_cast<float>(yOffset) + static_cast<float>(data.m_offsetY) * fontSize / m_maxFontSize,
            static_cast<float>(fontSize) / static_cast<float>(m_maxFontSize),
            WHITE
        };

        xOffset += static_cast<uint32_t>((kerning + data.m_width) * scaleFactor);
    }
}

moraine::Font_I::FontChar moraine::Font_I::findOrAllocChar(wchar_t character) const
{
    auto searchResult = m_charData.find(character);

    if (searchResult != m_charData.end())
        return searchResult->second;

    int c_x1, c_y1, c_x2, c_y2;
    stbtt_GetCodepointBitmapBox(&m_fontInfo, character, m_stbFontSize, m_stbFontSize, &c_x1, &c_y1, &c_x2, &c_y2);

    uint32_t width = c_x2 - c_x1;
    uint32_t height = c_y2 - c_y1;

    FontChar c;
    c.m_offsetX = c_x1;
    c.m_offsetY = c_y1;

    stbtt_MakeCodepointBitmap(&m_fontInfo,
                              static_cast<uint8_t*>(m_atlas->allocateImageSpace(width, height, &c.m_spriteSheetLocation)),
                              width, height,
                              width,
                              m_stbFontSize, m_stbFontSize,
                              character);

    int charWidth;
    stbtt_GetCodepointHMetrics(&m_fontInfo, character, &charWidth, 0);
    c.m_width = static_cast<uint32_t>(charWidth);

    return (*m_charData.insert(std::pair<wchar_t, FontChar>(character, c)).first).second;
}


moraine::Font moraine::createFont(GraphicsContext context, Stringr ttfFile, uint32_t maxPixelHeight)
{
    return std::make_shared<Font_I>(context, ttfFile, maxPixelHeight);
}

std::weak_ptr<moraine::Shader_T> moraine::Font_I::s_fontShader;