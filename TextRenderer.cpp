#include "TextRenderer.h"

#include <algorithm>
#include <cmath>

namespace asset
{
    TextRenderer::~TextRenderer()
    {
        Shutdown();
    }

    TextRenderer::TextRenderer(TextRenderer&& other) noexcept
        : m_vao(other.m_vao)
        , m_vbo(other.m_vbo)
        , m_capacityVerts(other.m_capacityVerts)
        , m_cpuVerts(std::move(other.m_cpuVerts))
        , m_cache(std::move(other.m_cache))
    {
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_capacityVerts = 0;
    }

    TextRenderer& TextRenderer::operator=(TextRenderer&& other) noexcept
    {
        if (this != &other)
        {
            Shutdown();

            m_vao = other.m_vao;
            m_vbo = other.m_vbo;
            m_capacityVerts = other.m_capacityVerts;
            m_cpuVerts = std::move(other.m_cpuVerts);
            m_cache = std::move(other.m_cache);

            other.m_vao = 0;
            other.m_vbo = 0;
            other.m_capacityVerts = 0;
        }
        return *this;
    }

    void TextRenderer::Initialize()
    {
        if (m_vao != 0)
        {
            return;
        }

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        m_capacityVerts = 256;
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_capacityVerts * sizeof(TextVertex)),
            nullptr,
            GL_DYNAMIC_DRAW);

        // layout(location=0) vec3 aPos
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
            reinterpret_cast<void*>(offsetof(TextVertex, Pos)));

        // layout(location=1) vec2 aUV
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
            reinterpret_cast<void*>(offsetof(TextVertex, UV)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void TextRenderer::Shutdown()
    {
        if (m_vbo != 0)
        {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_vao != 0)
        {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }

        m_capacityVerts = 0;
        m_cpuVerts.clear();
        m_cache.clear();
    }

    void TextRenderer::ClearCache()
    {
        m_cache.clear();
    }

    void TextRenderer::EnsureBufferCapacity(size_t vertexCount)
    {
        if (vertexCount <= m_capacityVerts)
        {
            return;
        }

        m_capacityVerts = std::max(vertexCount, m_capacityVerts * 2);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m_capacityVerts * sizeof(TextVertex)),
            nullptr,
            GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void TextRenderer::ComputeBillboardBasis(const glm::vec3& camPos,
        const glm::vec3& worldPos,
        glm::vec3& outRight,
        glm::vec3& outUp)
    {
        const glm::vec3 forward = glm::normalize(camPos - worldPos); // label -> camera
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        outRight = glm::cross(worldUp, forward);
        const float lenR = glm::length(outRight);
        if (lenR < 1e-6f)
        {
            outRight = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            outRight /= lenR;
        }

        outUp = glm::normalize(glm::cross(forward, outRight));
    }

    TextRenderer::CachedText TextRenderer::BuildCacheForString(const FontAtlas& font, std::string_view text)
    {
        CachedText out{};
        out.Valid = false;

        if (text.empty() || font.TextureId() == 0)
        {
            return out;
        }

        // First pass: compute bounds in baked-pixel units (scale=1)
        bool first = true;
        float penX = 0.0f;
        float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;

        for (char c : text)
        {
            if (!font.HasGlyph(c))
            {
                continue;
            }

            const auto& g = font.GetGlyph(c).Baked;

            const float x0 = penX + g.xoff;
            const float y0 = g.yoff;
            const float w = static_cast<float>(g.x1 - g.x0);
            const float h = static_cast<float>(g.y1 - g.y0);

            const float x1 = x0 + w;
            const float y1 = y0 + h;

            if (first)
            {
                minX = x0; minY = y0; maxX = x1; maxY = y1;
                first = false;
            }
            else
            {
                minX = std::min(minX, x0);
                minY = std::min(minY, y0);
                maxX = std::max(maxX, x1);
                maxY = std::max(maxY, y1);
            }

            penX += g.xadvance;
        }

        if (first)
        {
            return out;
        }

        const float centerX = (minX + maxX) * 0.5f;
        const float centerY = (minY + maxY) * 0.5f;

        // Second pass: store centered quads + UVs
        out.Quads.clear();
        out.Quads.reserve(text.size());

        penX = 0.0f;

        for (char c : text)
        {
            if (!font.HasGlyph(c))
            {
                continue;
            }

            const auto& g = font.GetGlyph(c).Baked;

            const float x0 = (penX + g.xoff) - centerX;
            const float y0 = (g.yoff) - centerY;
            const float w = static_cast<float>(g.x1 - g.x0);
            const float h = static_cast<float>(g.y1 - g.y0);

            const float x1 = x0 + w;
            const float y1 = y0 + h;

            const float u0 = static_cast<float>(g.x0) / static_cast<float>(font.AtlasWidth());
            const float v0 = static_cast<float>(g.y0) / static_cast<float>(font.AtlasHeight());
            const float u1 = static_cast<float>(g.x1) / static_cast<float>(font.AtlasWidth());
            const float v1 = static_cast<float>(g.y1) / static_cast<float>(font.AtlasHeight());

            out.Quads.push_back(CachedQuad{ x0, y0, x1, y1, u0, v0, u1, v1 });

            penX += g.xadvance;
        }

        out.Valid = !out.Quads.empty();
        return out;
    }

    const TextRenderer::CachedText& TextRenderer::GetOrBuildCache(const FontAtlas& font, std::string_view text)
    {
        // NOTE:
        // Cache key is the string. This assumes you're using the same FontAtlas glyph set for these draws.
        // If you plan to use multiple fonts/atlases, extend the key (e.g., include font.TextureId()).
        const std::string key(text);

        auto it = m_cache.find(key);
        if (it != m_cache.end())
        {
            return it->second;
        }

        CachedText built = BuildCacheForString(font, text);
        auto [insIt, _] = m_cache.emplace(key, std::move(built));
        return insIt->second;
    }

    void TextRenderer::DrawTextBillboard(const FontAtlas& font,
        Shader& textShader,
        std::string_view text,
        const glm::vec3& worldPos,
        const Camera& camera,
        const glm::mat4& view,
        const glm::mat4& proj,
        int viewportWidth,
        int viewportHeight,
        float pixelHeight,
        const glm::vec4& color,
        bool depthTest)
    {
        (void)viewportWidth;

        if (text.empty() || font.TextureId() == 0 || viewportHeight <= 0)
        {
            return;
        }

        if (m_vao == 0)
        {
            Initialize();
        }

        const CachedText& cached = GetOrBuildCache(font, text);
        if (!cached.Valid)
        {
            return;
        }

        glm::vec3 right, up;
        ComputeBillboardBasis(camera.Position(), worldPos, right, up);

        const float bakeH = std::max(font.BakePixelHeight(), 1.0f);
        const float scalePx = std::max(pixelHeight, 1.0f) / bakeH;

        // World units per screen pixel at distance
        const float dist = glm::length(camera.Position() - worldPos);
        const float halfFov = camera.FovYRadians() * 0.5f;
        const float worldPerPixel = (2.0f * dist * std::tan(halfFov)) / static_cast<float>(viewportHeight);
        const float pxToWorld = worldPerPixel;

        // Build world-space vertices from cached centered quads
        m_cpuVerts.clear();
        m_cpuVerts.reserve(cached.Quads.size() * 6);

        for (const CachedQuad& q : cached.Quads)
        {
            // Scale centered baked-pixel quad into desired screen pixels
            const float x0p = q.x0 * scalePx;
            const float y0p = q.y0 * scalePx;
            const float x1p = q.x1 * scalePx;
            const float y1p = q.y1 * scalePx;

            // baked y increases downward -> flip to world up by negating y contribution
            const glm::vec3 p00 = worldPos + right * (x0p * pxToWorld) + up * (-y0p * pxToWorld);
            const glm::vec3 p10 = worldPos + right * (x1p * pxToWorld) + up * (-y0p * pxToWorld);
            const glm::vec3 p11 = worldPos + right * (x1p * pxToWorld) + up * (-y1p * pxToWorld);
            const glm::vec3 p01 = worldPos + right * (x0p * pxToWorld) + up * (-y1p * pxToWorld);

            // Two triangles: (p00, p10, p11) (p00, p11, p01)
            m_cpuVerts.push_back({ p00, {q.u0, q.v0} });
            m_cpuVerts.push_back({ p10, {q.u1, q.v0} });
            m_cpuVerts.push_back({ p11, {q.u1, q.v1} });

            m_cpuVerts.push_back({ p00, {q.u0, q.v0} });
            m_cpuVerts.push_back({ p11, {q.u1, q.v1} });
            m_cpuVerts.push_back({ p01, {q.u0, q.v1} });
        }

        if (m_cpuVerts.empty())
        {
            return;
        }

        EnsureBufferCapacity(m_cpuVerts.size());

        const GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
        if (depthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glDepthMask(GL_FALSE);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(m_cpuVerts.size() * sizeof(TextVertex)),
            m_cpuVerts.data());

        textShader.Use();
        textShader.SetMat4("uView", view);
        textShader.SetMat4("uProj", proj);
        textShader.SetVec4("uColor", color);
        textShader.SetInt("uFontAtlas", 0);

        font.Bind(0);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_cpuVerts.size()));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDepthMask(depthMask);

        if (wasDepthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }
} // namespace gfx
