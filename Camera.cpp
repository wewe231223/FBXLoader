#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace asset
{
    void Camera::SetAspect(float aspect)
    {
        m_aspect = std::max(0.001f, aspect);
    }

    void Camera::SetTarget(const glm::vec3& target)
    {
        m_target = target;
    }

    void Camera::SetDistance(float distance)
    {
        m_distance = distance;
        Clamp();
    }

    void Camera::SetYawPitch(float yawRadians, float pitchRadians)
    {
        m_yaw = yawRadians;
        m_pitch = pitchRadians;
        Clamp();
    }

    void Camera::AddYawPitch(float yawDeltaRadians, float pitchDeltaRadians)
    {
        m_yaw += yawDeltaRadians;
        m_pitch += pitchDeltaRadians;
        Clamp();
    }

    void Camera::AddZoom(float zoomDelta)
    {
        // zoomDelta > 0 -> zoom in (distance smaller)
        m_distance -= zoomDelta;
        Clamp();
    }

    void Camera::Clamp()
    {
        const float eps = glm::radians(0.1f);
        const float limit = glm::half_pi<float>() - eps;

        m_pitch = std::clamp(m_pitch, -limit, limit);
        m_distance = std::clamp(m_distance, 0.5f, 100.0f);
    }

    glm::vec3 Camera::Position() const
    {
        // Spherical orbit around target.
        const float cy = cosf(m_yaw);
        const float sy = sinf(m_yaw);
        const float cp = cosf(m_pitch);
        const float sp = sinf(m_pitch);

        // Forward from camera to target (in world)
        const glm::vec3 dirToTarget = glm::normalize(glm::vec3(cp * sy, sp, cp * cy));
        return m_target - dirToTarget * m_distance;
    }

    glm::mat4 Camera::View() const
    {
        return glm::lookAt(Position(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 Camera::Proj() const
    {
        return glm::perspective(m_fovY, m_aspect, m_nearZ, m_farZ);
    }

    float Camera::FovYRadians() const
    {
        return m_fovY;
    }

    void Camera::FrameBounds(const glm::vec3& center, float radius, float fitMargin)
    {
        SetTarget(center);

        // radius가 0에 가까운 경우 대비
        const float r = std::max(radius, 0.001f);

        // 수직 FOV 기준으로, 구가 화면에 들어오려면:
        // distance >= r / sin(fov/2)
        const float halfFov = m_fovY * 0.5f;
        const float dist = (r * fitMargin) / std::sin(std::max(halfFov, 0.001f));

        SetDistance(dist);
    }

} // namespace gfx
