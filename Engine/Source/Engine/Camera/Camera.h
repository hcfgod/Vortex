#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/gtx/euler_angles.hpp>
#include "Engine/Systems/EngineSystem.h"
#include "Core/Debug/ErrorCodes.h"

namespace Vortex 
{
    // ------------------------------------------------------------
    // Base Camera interface
    // ------------------------------------------------------------
    class Camera 
    {
    public:
        virtual ~Camera() = default;

        // -----------------------------------------------------------------
        // Optional per-frame update.  Cameras that need dynamic behaviour
        // (e.g., EditorCamera responding to input) override this.  The base
        // implementation is a no-op so that static cameras can ignore it.
        // -----------------------------------------------------------------
        virtual void OnUpdate(float /*deltaTime*/) {}

        // View / Projection access ------------------------------------------
        virtual const glm::mat4& GetViewMatrix()       const = 0;
        virtual const glm::mat4& GetProjectionMatrix() const = 0;

        // Utility -------------------------------------------------------------
        glm::mat4 GetViewProjectionMatrix() const { return GetProjectionMatrix() * GetViewMatrix(); }

        // Called when the target surface / window changes size.
        virtual void SetViewportSize(uint32_t width, uint32_t height) = 0;
    };

    // ------------------------------------------------------------
    // PerspectiveCamera (default 3D camera)
    // ------------------------------------------------------------
    class PerspectiveCamera : public Camera 
    {
    public:
        PerspectiveCamera(float fov = 45.0f, float aspectRatio = 16.f / 9.f,
            float nearClip = 0.1f, float farClip = 1000.f)
            : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
        {
            RecalculateProjection();
            RecalculateViewMatrix();
        }

        // --- Camera control --------------------------------------------------
        void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
        void SetRotation(const glm::vec3& eulerDegrees) { m_Rotation = eulerDegrees; RecalculateViewMatrix(); }

        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetRotation() const { return m_Rotation; }

        // LookAt helper
        void LookAt(const glm::vec3& target) {
            m_View = glm::lookAt(m_Position, target, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // --------------------------------------------------------------------
        void SetViewportSize(uint32_t width, uint32_t height) override {
            if (height == 0) height = 1; // avoid division by zero
            m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
            RecalculateProjection();
        }

        // --------------------------------------------------------------------
        const glm::mat4& GetViewMatrix() const override { return m_View; }
        const glm::mat4& GetProjectionMatrix() const override { return m_Projection; }

    private:
        // Recalculations ------------------------------------------------------
        void RecalculateProjection() {
            m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        }
        void RecalculateViewMatrix() {
            // Convert Euler (pitch yaw roll) to direction vectors via transform composition
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
                glm::yawPitchRoll(glm::radians(m_Rotation.y), glm::radians(m_Rotation.x), glm::radians(m_Rotation.z));
            m_View = glm::inverse(transform);
        }

        // Members -------------------------------------------------------------
        float m_FOV;
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;

        glm::vec3 m_Position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 m_Rotation{ 0.0f, 0.0f, 0.0f }; // pitch, yaw, roll (deg)

        glm::mat4 m_View{ 1.0f };
        glm::mat4 m_Projection{ 1.0f };
    };

    // ------------------------------------------------------------
    // OrthographicCamera (2D UI / editor mode)
    // ------------------------------------------------------------
    class OrthographicCamera : public Camera 
    {
    public:
        OrthographicCamera(float left = -1.f, float right = 1.f,
            float bottom = -1.f, float top = 1.f,
            float nearClip = -1.f, float farClip = 1.f)
            : m_Left(left), m_Right(right), m_Bottom(bottom), m_Top(top),
            m_NearClip(nearClip), m_FarClip(farClip)
        {
            RecalculateProjection();
            RecalculateViewMatrix();
        }

        // Projection config ---------------------------------------------------
        void SetProjection(float left, float right, float bottom, float top, float nearClip, float farClip) {
            m_Left = left; m_Right = right; m_Bottom = bottom; m_Top = top; m_NearClip = nearClip; m_FarClip = farClip;
            RecalculateProjection();
        }

        // Camera control ------------------------------------------------------
        void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
        void SetRotation(float rotationDeg) { m_Rotation = rotationDeg; RecalculateViewMatrix(); }

        const glm::vec3& GetPosition() const { return m_Position; }
        float GetRotation() const { return m_Rotation; }

        // Viewport ------------------------------------------------------------
        void SetViewportSize(uint32_t width, uint32_t height) override {
            float aspect = static_cast<float>(width) / (height == 0 ? 1.0f : static_cast<float>(height));
            float orthoHeight = (m_Top - m_Bottom);
            float orthoWidth = orthoHeight * aspect;
            float centerX = (m_Left + m_Right) * 0.5f;
            m_Left = centerX - orthoWidth * 0.5f;
            m_Right = centerX + orthoWidth * 0.5f;
            RecalculateProjection();
        }

        // Matrices ------------------------------------------------------------
        const glm::mat4& GetViewMatrix() const override { return m_View; }
        const glm::mat4& GetProjectionMatrix() const override { return m_Projection; }

    private:
        void RecalculateProjection() {
            m_Projection = glm::ortho(m_Left, m_Right, m_Bottom, m_Top, m_NearClip, m_FarClip);
        }

        void RecalculateViewMatrix() {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
                glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));
            m_View = glm::inverse(transform);
        }

        // Members -------------------------------------------------------------
        float m_Left, m_Right, m_Bottom, m_Top;
        float m_NearClip, m_FarClip;

        glm::vec3 m_Position{ 0.0f, 0.0f, 0.0f };
        float     m_Rotation{ 0.0f }; // z-rotation only

        glm::mat4 m_View{ 1.0f };
        glm::mat4 m_Projection{ 1.0f };
    };

    // ============================================================
    // CameraSystem - EngineSystem-based camera management
    // ============================================================
    class CameraSystem : public EngineSystem
    {
    public:
        CameraSystem();
        ~CameraSystem() override;

        // EngineSystem interface
        Result<void> Initialize() override;
        Result<void> Update() override;
        Result<void> Render() override { return Result<void>(); }
        Result<void> Shutdown() override;

        // Camera management
        void Register(const std::shared_ptr<Camera>& camera);
        void Unregister(const std::shared_ptr<Camera>& camera);

        const std::vector<std::shared_ptr<Camera>>& GetCameras() const { return m_Cameras; }

        std::shared_ptr<Camera> GetActiveCamera() const { return m_ActiveCamera; }
        void SetActiveCamera(const std::shared_ptr<Camera>& camera) { m_ActiveCamera = camera; }

        // Window resize handling
        void OnWindowResize(uint32_t width, uint32_t height);

    private:
        std::vector<std::shared_ptr<Camera>> m_Cameras;
        std::shared_ptr<Camera> m_ActiveCamera;
    };

} // namespace Vortex 