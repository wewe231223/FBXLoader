#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "FontAtlas.h"
#include "Shader.h"
#include "Camera.h"

namespace asset
{
    class TextRenderer
    {
    public:
        TextRenderer() = default;
        ~TextRenderer();

        TextRenderer(const TextRenderer&) = delete;
        TextRenderer& operator=(const TextRenderer&) = delete;

        TextRenderer(TextRenderer&& other) noexcept;
        TextRenderer& operator=(TextRenderer&& other) noexcept;

        void Initialize();
        void Shutdown();

        // Optional: clear cached string->quad conversions
        void ClearCache();

        // World-space billboard text:
        // - Faces the camera
        // - Approx constant screen size using camera FOV + distance (pixelHeight)
        // - Anchored at worldPos (text is centered around it)
        // - Requires blending enabled (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
        void DrawTextBillboard(const FontAtlas& font,
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
            bool depthTest = true);

    private:
        struct TextVertex
        {
            glm::vec3 Pos;
            glm::vec2 UV;
        };

        // Cached, centered (origin at text center) in baked-pixel units (scale=1.0)
        struct CachedQuad
        {
            float x0{};
            float y0{};
            float x1{};
            float y1{};
            float u0{};
            float v0{};
            float u1{};
            float v1{};
        };

        struct CachedText
        {
            std::vector<CachedQuad> Quads;
            bool Valid{ false };
        };

        static void ComputeBillboardBasis(const glm::vec3& camPos,
            const glm::vec3& worldPos,
            glm::vec3& outRight,
            glm::vec3& outUp);

        void EnsureBufferCapacity(size_t vertexCount);

        // Build (or fetch) cached quads for a given string.
        // Cache is per TextRenderer instance, and assumes the same FontAtlas (baked set) is used.
        const CachedText& GetOrBuildCache(const FontAtlas& font, std::string_view text);

        static CachedText BuildCacheForString(const FontAtlas& font, std::string_view text);

    private:
        GLuint m_vao{ 0 };
        GLuint m_vbo{ 0 };

        size_t m_capacityVerts{ 0 };
        std::vector<TextVertex> m_cpuVerts;

        std::unordered_map<std::string, CachedText> m_cache;
    };
} // namespace gfx
