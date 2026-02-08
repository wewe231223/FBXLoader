#pragma once

#include <glm/glm.hpp>

namespace asset
{
    class Renderer
    {
    public:
        void Initialize();
        void Resize(int width, int height);

        void BeginFrame(const glm::vec4& clearColor) const;
        void EndFrame() const;

    private:
        int m_width{1280};
        int m_height{720};
    };
} // namespace gfx
