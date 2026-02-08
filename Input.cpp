#include "Input.h"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace asset
{
    Input::Input(GLFWwindow* window)
        : m_window(window)
    {
        m_keyDown.fill(0);
        m_keyPressed.fill(0);
        m_mouseDown.fill(0);
        m_mousePressed.fill(0);
    }

    void Input::InstallCallbacks(GLFWwindow* window, Input* input)
    {
        glfwSetWindowUserPointer(window, input);

        glfwSetKeyCallback(window, &Input::KeyCallback);
        glfwSetMouseButtonCallback(window, &Input::MouseButtonCallback);
        glfwSetCursorPosCallback(window, &Input::CursorPosCallback);
        glfwSetScrollCallback(window, &Input::ScrollCallback);

        // 추가: 파일 드롭 콜백
        glfwSetDropCallback(window, &Input::DropCallback);
    }

    void Input::NewFrame()
    {
        m_keyPressed.fill(0);
        m_mousePressed.fill(0);
        m_mouseDelta = glm::vec2(0.0f);
        m_scrollDelta = 0.0f;
        // 드롭 파일은 ConsumeDroppedFiles()로 소비하는 방식이므로 여기서 지우지 않습니다.
    }

    void Input::Poll()
    {
        glfwPollEvents();
    }

    bool Input::KeyDown(int key) const
    {
        if (key < 0 || key >= KeyCount)
        {
            return false;
        }
        return m_keyDown[static_cast<size_t>(key)] != 0;
    }

    bool Input::KeyPressed(int key) const
    {
        if (key < 0 || key >= KeyCount)
        {
            return false;
        }
        return m_keyPressed[static_cast<size_t>(key)] != 0;
    }

    bool Input::MouseDown(int button) const
    {
        if (button < 0 || button >= MouseButtonCount)
        {
            return false;
        }
        return m_mouseDown[static_cast<size_t>(button)] != 0;
    }

    bool Input::MousePressed(int button) const
    {
        if (button < 0 || button >= MouseButtonCount)
        {
            return false;
        }
        return m_mousePressed[static_cast<size_t>(button)] != 0;
    }

    glm::vec2 Input::MousePos() const
    {
        return m_mousePos;
    }

    glm::vec2 Input::MouseDelta() const
    {
        return m_mouseDelta;
    }

    float Input::ScrollDelta() const
    {
        return m_scrollDelta;
    }

    std::vector<std::string> Input::ConsumeDroppedFiles()
    {
        std::vector<std::string> out = std::move(m_droppedFiles);
        m_droppedFiles.clear();
        m_droppedFiles.shrink_to_fit(); // 원하면 제거해도 됨(할당 유지하고 싶으면 삭제)
        return out;
    }

    void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        (void)scancode;
        (void)mods;

        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input)
        {
            return;
        }
        input->OnKey(key, action);
    }

    void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        (void)mods;

        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input)
        {
            return;
        }
        input->OnMouseButton(button, action);
    }

    void Input::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input)
        {
            return;
        }
        input->OnCursorPos(xpos, ypos);
    }

    void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        (void)xoffset;

        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input)
        {
            return;
        }
        input->OnScroll(yoffset);
    }

    void Input::DropCallback(GLFWwindow* window, int count, const char** paths)
    {
        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input)
        {
            return;
        }
        input->OnDrop(count, paths);
    }

    void Input::OnKey(int key, int action)
    {
        if (key < 0 || key >= KeyCount)
        {
            return;
        }

        const size_t idx = static_cast<size_t>(key);

        if (action == GLFW_PRESS)
        {
            if (m_keyDown[idx] == 0)
            {
                m_keyPressed[idx] = 1;
            }
            m_keyDown[idx] = 1;
        }
        else if (action == GLFW_RELEASE)
        {
            m_keyDown[idx] = 0;
        }
    }

    void Input::OnMouseButton(int button, int action)
    {
        if (button < 0 || button >= MouseButtonCount)
        {
            return;
        }

        const size_t idx = static_cast<size_t>(button);

        if (action == GLFW_PRESS)
        {
            if (m_mouseDown[idx] == 0)
            {
                m_mousePressed[idx] = 1;
            }
            m_mouseDown[idx] = 1;
        }
        else if (action == GLFW_RELEASE)
        {
            m_mouseDown[idx] = 0;
        }
    }

    void Input::OnCursorPos(double xpos, double ypos)
    {
        m_mousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

        if (m_firstMouse)
        {
            m_prevMousePos = m_mousePos;
            m_firstMouse = false;
        }

        m_mouseDelta = m_mousePos - m_prevMousePos;
        m_prevMousePos = m_mousePos;
    }

    void Input::OnScroll(double yoffset)
    {
        m_scrollDelta += static_cast<float>(yoffset);
    }

    void Input::OnDrop(int count, const char** paths)
    {
        if (count <= 0 || paths == nullptr)
        {
            return;
        }

        m_droppedFiles.reserve(m_droppedFiles.size() + static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            if (paths[i] != nullptr)
            {
                m_droppedFiles.emplace_back(paths[i]);
            }
        }
    }
} // namespace gfx
