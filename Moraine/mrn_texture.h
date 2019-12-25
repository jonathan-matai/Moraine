#pragma once

#include "mrn_gfxcontext.h"
#include "mrn_constset.h"

namespace moraine
{
    enum ImageColorChannels
    {
        IMAGE_COLOR_CHANNELS_BW = 1,
        IMAGE_COLOR_CHANNELS_BW_ALPHA = 2,
        IMAGE_COLOR_CHANNELS_RGB = 3,
        IMAGE_COLOR_CHANNLES_RGB_ALPHA = 4
    };

    enum TextureFlags
    {
        UNNORMALIZED_UV_COORDINATES = 0b01
    };

    struct CopySubImage
    {
        void* stagingStackLocation;
        RectangleU imageLocation;
    };

    class Texture_T : public ConstantResource_T
    {
    public:

        Texture_T(uint32_t width, uint32_t height) :
            ConstantResource_T(CONSTANT_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER),
            m_width(width),
            m_height(height)
        { }

        virtual ~Texture_T() = default;

        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }

        virtual void resize(uint32_t width, uint32_t height) = 0;

        virtual void copyBufferRegionsToTexture(std::vector<CopySubImage>& regions, std::function<void()> operationOnComplete) = 0;

    protected:

        uint32_t m_width;
        uint32_t m_height;
    };

    typedef std::shared_ptr<Texture_T> Texture;

    MRN_API Texture createTexture(GraphicsContext context, Stringr imagePath, uint32_t textureFlags = 0);
    MRN_API Texture createTexture(GraphicsContext context, ImageColorChannels channels, uint32_t width, uint32_t height, uint32_t textureFlags = 0);

    class TextureAtlas_T
    {
    public:

        virtual ~TextureAtlas_T() = default;

        Texture getTexture() const { return m_texture; }

        virtual void* allocateImageSpace(uint32_t width, uint32_t height, RectangleU* out_location = nullptr) = 0;
        virtual void addImageSpacesToAtlas() = 0;

    protected:

        Texture m_texture;
    };

    typedef std::shared_ptr<TextureAtlas_T> TextureAtlas;

    MRN_API TextureAtlas createTextureAtlas(GraphicsContext context, ImageColorChannels channels, uint32_t initialSize);
}