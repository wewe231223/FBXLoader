#include "FontAtlas.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <fstream>
#include <algorithm>

namespace asset
{
    FontAtlas::~FontAtlas()
    {
        Destroy();
    }

    FontAtlas::FontAtlas(FontAtlas&& other) noexcept
        : m_tex(other.m_tex)
        , m_atlasW(other.m_atlasW)
        , m_atlasH(other.m_atlasH)
        , m_bakeHeight(other.m_bakeHeight)
        , m_firstChar(other.m_firstChar)
        , m_charCount(other.m_charCount)
        , m_ttfBytes(std::move(other.m_ttfBytes))
        , m_atlasPixels(std::move(other.m_atlasPixels))
        , m_glyphs(std::move(other.m_glyphs))
    {
        other.m_tex = 0;
        other.m_atlasW = 0;
        other.m_atlasH = 0;
        other.m_bakeHeight = 0.0f;
        other.m_firstChar = 32;
        other.m_charCount = 0;
    }

    FontAtlas& FontAtlas::operator=(FontAtlas&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();

            m_tex = other.m_tex;
            m_atlasW = other.m_atlasW;
            m_atlasH = other.m_atlasH;
            m_bakeHeight = other.m_bakeHeight;
            m_firstChar = other.m_firstChar;
            m_charCount = other.m_charCount;
            m_ttfBytes = std::move(other.m_ttfBytes);
            m_atlasPixels = std::move(other.m_atlasPixels);
            m_glyphs = std::move(other.m_glyphs);

            other.m_tex = 0;
            other.m_atlasW = 0;
            other.m_atlasH = 0;
            other.m_bakeHeight = 0.0f;
            other.m_firstChar = 32;
            other.m_charCount = 0;
        }
        return *this;
    }

    void FontAtlas::Destroy()
    {
        if (m_tex != 0)
        {
            glDeleteTextures(1, &m_tex);
            m_tex = 0;
        }

        m_atlasW = 0;
        m_atlasH = 0;
        m_bakeHeight = 0.0f;
        m_firstChar = 32;
        m_charCount = 0;
        m_ttfBytes.clear();
        m_atlasPixels.clear();
        m_glyphs.clear();
    }

    static bool ReadAllBytes(const std::string& path, std::vector<uint8_t>& outBytes)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
        {
            return false;
        }

        ifs.seekg(0, std::ios::end);
        const std::streamoff size = ifs.tellg();
        if (size <= 0)
        {
            return false;
        }

        outBytes.resize(static_cast<size_t>(size));
        ifs.seekg(0, std::ios::beg);
        ifs.read(reinterpret_cast<char*>(outBytes.data()), size);
        return true;
    }

    bool FontAtlas::LoadFromTtfFile(const std::string& ttfPath,
                                    float bakePixelHeight,
                                    int atlasWidth,
                                    int atlasHeight,
                                    int firstChar,
                                    int charCount)
    {
        Destroy();

        if (atlasWidth <= 0 || atlasHeight <= 0 || bakePixelHeight <= 0.0f || charCount <= 0)
        {
            return false;
        }

        if (!ReadAllBytes(ttfPath, m_ttfBytes))
        {
            return false;
        }

        m_atlasW = atlasWidth;
        m_atlasH = atlasHeight;
        m_bakeHeight = bakePixelHeight;
        m_firstChar = firstChar;
        m_charCount = charCount;

        m_atlasPixels.assign(static_cast<size_t>(m_atlasW * m_atlasH), 0u);
        m_glyphs.resize(static_cast<size_t>(m_charCount));

        std::vector<stbtt_bakedchar> baked(static_cast<size_t>(m_charCount));

        const int ok = stbtt_BakeFontBitmap(
            m_ttfBytes.data(),
            0,
            m_bakeHeight,
            m_atlasPixels.data(),
            m_atlasW,
            m_atlasH,
            m_firstChar,
            m_charCount,
            baked.data()
        );

        if (ok <= 0)
        {
            Destroy();
            return false;
        }

        for (int idx = 0; idx < m_charCount; ++idx)
        {
            m_glyphs[static_cast<size_t>(idx)].Baked = baked[static_cast<size_t>(idx)];
            m_glyphs[static_cast<size_t>(idx)].Valid = true;
        }

        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_R8,
                     m_atlasW,
                     m_atlasH,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     m_atlasPixels.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Swizzle R -> A so we can read alpha directly.
        GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void FontAtlas::Bind(unsigned int unit) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_tex);
    }

    GLuint FontAtlas::TextureId() const { return m_tex; }
    int FontAtlas::AtlasWidth() const { return m_atlasW; }
    int FontAtlas::AtlasHeight() const { return m_atlasH; }
    float FontAtlas::BakePixelHeight() const { return m_bakeHeight; }
    int FontAtlas::FirstChar() const { return m_firstChar; }
    int FontAtlas::CharCount() const { return m_charCount; }

    const FontAtlas::Glyph& FontAtlas::GetGlyph(char c) const
    {
        static Glyph dummy{};

        const int code = static_cast<unsigned char>(c);
        const int idx = code - m_firstChar;
        if (idx < 0 || idx >= m_charCount)
        {
            return dummy;
        }

        return m_glyphs[static_cast<size_t>(idx)];
    }

    bool FontAtlas::HasGlyph(char c) const
    {
        const auto& g = GetGlyph(c);
        return g.Valid;
    }
} // namespace gfx
