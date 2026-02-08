#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct GLFWwindow;

namespace asset
{
    class Input
    {
    public:
        explicit Input(GLFWwindow* window);

        void NewFrame();
        void Poll(); // glfwPollEvents()는 main에서 호출해도 되지만, 여기서도 가능

        bool KeyDown(int key) const;
        bool KeyPressed(int key) const;

        bool MouseDown(int button) const;
        bool MousePressed(int button) const;

        glm::vec2 MousePos() const;
        glm::vec2 MouseDelta() const;

        float ScrollDelta() const;

        // --- Drag & Drop ---
        // 한 프레임 동안 드롭된 파일 경로들을 반환하고 내부 큐를 비웁니다.
        std::vector<std::string> ConsumeDroppedFiles();

        static void InstallCallbacks(GLFWwindow* window, Input* input);

    private:
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

        // 추가: Drop callback
        static void DropCallback(GLFWwindow* window, int count, const char** paths);

        void OnKey(int key, int action);
        void OnMouseButton(int button, int action);
        void OnCursorPos(double xpos, double ypos);
        void OnScroll(double yoffset);

        // 추가: Drop handler
        void OnDrop(int count, const char** paths);

    private:
        GLFWwindow* m_window{ nullptr };

        static constexpr int KeyCount = 512;
        static constexpr int MouseButtonCount = 16;

        std::array<uint8_t, KeyCount> m_keyDown{};
        std::array<uint8_t, KeyCount> m_keyPressed{};

        std::array<uint8_t, MouseButtonCount> m_mouseDown{};
        std::array<uint8_t, MouseButtonCount> m_mousePressed{};

        glm::vec2 m_mousePos{ 0.0f };
        glm::vec2 m_prevMousePos{ 0.0f };
        glm::vec2 m_mouseDelta{ 0.0f };

        float m_scrollDelta{ 0.0f };
        bool m_firstMouse{ true };

        // --- Drag & Drop ---
        std::vector<std::string> m_droppedFiles;
    };
} // namespace gfx
