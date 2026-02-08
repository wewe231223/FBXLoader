#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glad/glad.h>

#include "stb_truetype.h"

namespace asset
{
    class FontAtlas
    {
    public:
        struct Glyph
        {
            stbtt_bakedchar Baked{};
            bool Valid{false};
        };

        FontAtlas() = default;
        ~FontAtlas();

        FontAtlas(const FontAtlas&) = delete;
        FontAtlas& operator=(const FontAtlas&) = delete;

        FontAtlas(FontAtlas&& other) noexcept;
        FontAtlas& operator=(FontAtlas&& other) noexcept;

        // Bakes [firstChar, firstChar + charCount) into a single grayscale atlas.
        // Recommended: ASCII 32..127 (first=32, count=96)
        bool LoadFromTtfFile(const std::string& ttfPath,
                             float bakePixelHeight,
                             int atlasWidth = 512,
                             int atlasHeight = 512,
                             int firstChar = 32,
                             int charCount = 96);

        void Bind(unsigned int unit) const;

        GLuint TextureId() const;
        int AtlasWidth() const;
        int AtlasHeight() const;
        float BakePixelHeight() const;

        int FirstChar() const;
        int CharCount() const;

        const Glyph& GetGlyph(char c) const;
        bool HasGlyph(char c) const;

    private:
        void Destroy();

    private:
        GLuint m_tex{0};

        int m_atlasW{0};
        int m_atlasH{0};
        float m_bakeHeight{0.0f};

        int m_firstChar{32};
        int m_charCount{96};

        std::vector<uint8_t> m_ttfBytes;
        std::vector<uint8_t> m_atlasPixels;
        std::vector<Glyph> m_glyphs;
    };
} // namespace gfx
