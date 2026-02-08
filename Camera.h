#pragma once

#include <glm/glm.hpp>

namespace asset
{
    class Camera
    {
    public:
        void SetAspect(float aspect);

        void SetTarget(const glm::vec3& target);
        void SetDistance(float distance);
        void SetYawPitch(float yawRadians, float pitchRadians);

        void AddYawPitch(float yawDeltaRadians, float pitchDeltaRadians);
        void AddZoom(float zoomDelta);

        glm::mat4 View() const;
        glm::mat4 Proj() const;

        glm::vec3 Position() const;

        float FovYRadians() const;

        void FrameBounds(const glm::vec3& center, float radius, float fitMargin = 1.2f);
    private:
        void Clamp();

    private:
        glm::vec3 m_target{0.0f, 0.0f, 0.0f};
        float m_distance{1000.0f};

        float m_yaw{0.0f};   // around Y
        float m_pitch{0.0f}; // up/down

        float m_fovY{glm::radians(60.0f)};
        float m_aspect{16.0f / 9.0f};
        float m_nearZ{0.1f};
        float m_farZ{5000.0f};
    };
} // namespace gfx
