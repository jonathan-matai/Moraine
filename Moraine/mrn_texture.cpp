#include "mrn_core.h"
#include "mrn_texture.h"

#include "mrn_texture_vk.h"

moraine::Texture moraine::createTexture(GraphicsContext context, Stringr imagePath, uint32_t textureFlags)
{
    return std::make_shared<Texture_IVulkan>(context, imagePath, textureFlags);
}

moraine::Texture moraine::createTexture(GraphicsContext context, ImageColorChannels channels, uint32_t width, uint32_t height, uint32_t textureFlags)
{
    return std::make_shared<Texture_IVulkan>(context, channels, width, height, textureFlags);
}



namespace moraine
{
    class TextureAtlas_I : public TextureAtlas_T
    {
    public:

        TextureAtlas_I(GraphicsContext context, ImageColorChannels channels, uint32_t initialSize);

        void* allocateImageSpace(uint32_t width, uint32_t height, RectangleU* out_location) override;
        void addImageSpacesToAtlas() override;

    private:

        GraphicsContext m_context;
        ImageColorChannels m_channels;
        
        struct Row
        {
            uint32_t yOffset;
            uint32_t height;
            uint32_t width;
        };

        std::vector<Row> m_rows;

        std::vector<CopySubImage> m_stagingSubImages;
    };

    TextureAtlas_I::TextureAtlas_I(GraphicsContext context, ImageColorChannels channels, uint32_t initialSize) :
        m_context(context),
        m_channels(channels)
    {
        m_texture = createTexture(context, channels, initialSize, initialSize, UNNORMALIZED_UV_COORDINATES);
    }

    void* TextureAtlas_I::allocateImageSpace(uint32_t width, uint32_t height, RectangleU* out_location)
    {
        void* allocation = m_context->m_stagingStack->alloc(width * height * m_channels, 4);

        if (m_rows.empty()) // Create row if no row exists
        {
            m_rows.push_back({ 0, height, width });
            m_stagingSubImages.push_back({ allocation, RectangleU(0, 0, width, height) });

            if (out_location)
                *out_location = RectangleU(0, 0, width, height);

            return allocation;
        }

        for (auto& a : m_rows)
            if (height < a.height and height > a.height * 3/5 and a.width + width < m_texture->getWidth()) // Find row that is same size or slightly larger and has space
            {
                m_stagingSubImages.push_back({ allocation, RectangleU(a.width, a.yOffset, width, height) });

                if (out_location)
                    *out_location = RectangleU(a.width, a.yOffset, width, height);

                a.width += width;
                return allocation;
            }

        extend_or_create_rows:

        auto& a = *m_rows.rbegin(); // Get last row

        if (height < a.height * 5/3 and height > a.height and a.yOffset + height < m_texture->getHeight()) // Check if last row can be made taller to fit image and check if texture has enough height
        {
            a.height = height;
            m_stagingSubImages.push_back({ allocation, RectangleU(a.width, a.yOffset, width, height) });

            if (out_location)
                *out_location = RectangleU(a.width, a.yOffset, width, height);

            a.width += width;
            return allocation;
        }

        uint32_t yOffset = a.yOffset + a.height;

        if (yOffset + height < m_texture->getHeight()) // Check if atlas has enough height
        {
            m_rows.push_back({ yOffset, height, width });
            m_stagingSubImages.push_back({ allocation, RectangleU(0, yOffset, width, height) });

            if (out_location)
                *out_location = RectangleU(0, yOffset, width, height);

            return allocation;
        }

        for (auto& a : m_rows)
            if (height < a.height and a.width + width < m_texture->getWidth())
            {
                m_stagingSubImages.push_back({ allocation, RectangleU(a.width, a.yOffset, width, height) });

                if (out_location)
                    *out_location = RectangleU(a.width, a.yOffset, width, height);

                a.width += width;
                return allocation;
            }

        m_texture->resize(m_texture->getWidth(), m_texture->getHeight() * 2);
        goto extend_or_create_rows; // Try again
        //return nullptr;
    }

    void TextureAtlas_I::addImageSpacesToAtlas()
    {
        if (m_stagingSubImages.empty())
            return;

        m_texture->copyBufferRegionsToTexture(m_stagingSubImages, nullptr);
        m_stagingSubImages.clear();
    }
}

moraine::TextureAtlas moraine::createTextureAtlas(GraphicsContext context, ImageColorChannels channels, uint32_t initialSize)
{
    return std::make_shared<TextureAtlas_I>(context, channels, initialSize);
}