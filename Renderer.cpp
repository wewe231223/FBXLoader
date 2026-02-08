#include "Renderer.h"

#include <glad/glad.h>

namespace asset
{
    void Renderer::Initialize()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    }

    void Renderer::Resize(int width, int height)
    {
        m_width = width;
        m_height = height;
        glViewport(0, 0, m_width, m_height);
    }

    void Renderer::BeginFrame(const glm::vec4& clearColor) const
    {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::EndFrame() const
    {
        // SwapBuffers는 main에서 처리 (GLFW)
    }
} // namespace gfx
