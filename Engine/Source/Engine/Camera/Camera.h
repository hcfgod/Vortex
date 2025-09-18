#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "Engine/Systems/EngineSystem.h"
#include "Core/Debug/ErrorCodes.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/KeyCodes.h"
#include "Core/Application.h"
#include "Engine/Engine.h"
#include "Engine/Systems/ImGuiInterop.h"
#include "Engine/Input/CursorControl.h"
#include "Core/Events/EventSystem.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/gtx/euler_angles.hpp>
#include <cmath>

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

    // ------------------------------------------------------------
    // EditorCamera - Interactive camera for editor mode
    // ------------------------------------------------------------
    class EditorCamera : public Camera 
    {
    public:
        enum class ProjectionType { Perspective = 0, Orthographic = 1 };

        EditorCamera(float verticalFOV = 45.0f, float aspectRatio = 16.0f/9.0f,
                     float nearClip = 0.1f, float farClip = 1000.0f)
            : m_VerticalFOV(verticalFOV), m_AspectRatio(aspectRatio),
              m_NearClip(nearClip), m_FarClip(farClip)
        {
            // Default orientation: looking down -Z
            m_Yaw        = 0.0f;
            m_TargetYaw  = m_Yaw;

            // Start slightly back so that origin is visible
            m_Position = {0.0f, 0.0f, 5.0f};

            RecalculateView();
            RecalculateProjection();
            
            // Subscribe to input events directly
            SubscribeToInputEvents();
        }
        
        ~EditorCamera()
        {
            UnsubscribeFromInputEvents();
            if (m_CursorCaptured)
            {
                if (auto* appPtr = Application::Get()) appPtr->SetRelativeMouseMode(false);
                m_CursorCaptured = false;
            }
        }

        // -----------------------------------------------------------------
        // Per-frame update (input driven)
        // -----------------------------------------------------------------
        void OnUpdate(float deltaTime) override
        {
            // Disable interactive editor camera behaviour when the engine is
            // running in Play or Pause states. It should only respond to input
            // while in Edit mode.
            auto* app = Application::Get();
            if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
                return;

            // Common drag-zoom (Shift + Left Mouse) --------------------------------
            HandleDragZoom();

            if(m_ProjectionType == ProjectionType::Perspective)
            {
                HandleMouseLook(deltaTime);
                HandleMovement(deltaTime);
                HandlePerspectiveZoom(); // Add mouse wheel zoom for perspective
            }
            else // Orthographic controls (Blender-style)
            {
                HandleOrthoPan(deltaTime);
                HandleOrthoZoom();

                // WASD movement similar to perspective but scaled by zoom level so
                // speed feels consistent regardless of ortho size.
                float originalSpeed = m_MoveSpeed;
                m_MoveSpeed = originalSpeed * (m_OrthoSize / 10.0f); // heuristic scale
                HandleMovement(deltaTime);
                m_MoveSpeed = originalSpeed; // restore

                // Rotation still available via Right mouse drag
                HandleMouseLook(deltaTime);
            }

            // Enable SDL relative mouse mode while manipulating camera to allow unbounded deltas
            {
                bool shift = m_KeysDown[static_cast<size_t>(KeyCode::LeftShift)] || m_KeysDown[static_cast<size_t>(KeyCode::RightShift)];

                auto manipulating = [&](bool sh){
                    return m_MouseButtonsDown[static_cast<size_t>(MouseCode::ButtonRight)] ||
                           (sh && m_MouseButtonsDown[static_cast<size_t>(MouseCode::ButtonLeft)]) ||
                           (sh && m_MouseButtonsDown[static_cast<size_t>(MouseCode::ButtonMiddle)]);
                };

                bool canStart = ImGuiViewportInput::IsHovered() && manipulating(shift);
                bool shouldContinue = manipulating(shift);
                bool targetCapture = m_CursorCaptured ? shouldContinue : canStart;

                if (targetCapture != m_CursorCaptured)
                {
                    m_CursorCaptured = targetCapture;
                    if (auto* appPtr = Application::Get())
                        appPtr->SetRelativeMouseMode(m_CursorCaptured);
                }
            }
            
            // Reset mouse deltas and scroll values after processing input
            m_MouseDX = 0.0f;
            m_MouseDY = 0.0f;
            m_MouseScrollX = 0.0f;
            m_MouseScrollY = 0.0f;
        }

        // -----------------------------------------------------------------
        // View / Projection access
        // -----------------------------------------------------------------
        const glm::mat4& GetViewMatrix() const override       { return m_View; }
        const glm::mat4& GetProjectionMatrix() const override { return m_Projection; }

        // -----------------------------------------------------------------
        // Viewport resize – keep aspect ratio current
        // -----------------------------------------------------------------
        void SetViewportSize(uint32_t width, uint32_t height) override
        {
            if(height == 0) height = 1;
            m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
            RecalculateProjection();
        }

        // -----------------------------------------------------------------
        // Projection switching / zoom
        // -----------------------------------------------------------------
        void SetProjectionType(ProjectionType type)
        {
            if (m_ProjectionType == type)
                return; // no-op

            ProjectionType prev = m_ProjectionType;
            m_ProjectionType = type;
            RecalculateProjection();

            // When switching from 2D (orthographic) to 3D (perspective) make sure
            // the camera is far enough from the origin so that the scene is not
            // clipped by the near plane. Mimic Unity's default view distance.
            if (prev == ProjectionType::Orthographic && type == ProjectionType::Perspective)
            {
                // Use ortho size as heuristic distance; clamp to minimum 10 units.
                float targetDist = std::max(m_OrthoSize, 10.0f);
                m_Position = { 0.0f, 0.0f, targetDist };
                m_Pitch = m_TargetPitch = 0.0f;
                m_Yaw   = m_TargetYaw   = 0.0f;
                RecalculateView();
            }
        }
        void ToggleProjection() { m_ProjectionType = m_ProjectionType == ProjectionType::Perspective ? ProjectionType::Orthographic : ProjectionType::Perspective; RecalculateProjection(); }

        ProjectionType GetProjectionType() const { return m_ProjectionType; }

        // Orthographic size control
        float GetOrthoSize() const { return m_OrthoSize; }
        void SetOrthoSize(float size) { m_OrthoSize = size; if (m_OrthoSize < 0.01f) m_OrthoSize = 0.01f; RecalculateProjection(); }

        // Camera transforms --------------------------------------------------
        const glm::vec3& GetPosition() const { return m_Position; }
        void SetPosition(const glm::vec3& pos) { m_Position = pos; RecalculateView(); }

        // Lightweight accessors for frustum parameters – needed for proper culling
        float GetVerticalFOV() const { return m_VerticalFOV; }
        float GetAspectRatio() const { return m_AspectRatio; }

    private:
        // Input event handling
        void SubscribeToInputEvents();
        void UnsubscribeFromInputEvents();
        bool OnKeyPressed(const KeyPressedEvent& event);
        bool OnKeyReleased(const KeyReleasedEvent& event);
        bool OnMouseMoved(const MouseMovedEvent& event);
        bool OnMouseButtonPressed(const MouseButtonPressedEvent& event);
        bool OnMouseButtonReleased(const MouseButtonReleasedEvent& event);
        bool OnMouseScrolled(const MouseScrolledEvent& event);

        // --------------------------------------------------------------------
        void HandleMouseLook(float dt)
        {
            // Check if right mouse button is held down
            if (!m_MouseButtonsDown[static_cast<size_t>(MouseCode::ButtonRight)])
            {
                m_FirstMouse = true;
                return;
            }

            if (m_FirstMouse)
            {
                m_FirstMouse = false;
                return; // Skip first frame to avoid jump
            }

            float offsetX = m_MouseDX;
            float offsetY = m_MouseDY;

            offsetX *= m_MouseSensitivity;
            offsetY *= m_MouseSensitivity;

            // Invert horizontal mouse to match typical Unity behaviour (drag right -> look right)
            m_TargetYaw   -= offsetX;
            m_TargetPitch += -offsetY; // invert so that moving mouse up looks up

            // Clamp target pitch
            const float pitchLimit = 89.0f;
            if (m_TargetPitch > pitchLimit)   m_TargetPitch = pitchLimit;
            if (m_TargetPitch < -pitchLimit)  m_TargetPitch = -pitchLimit;

            // Smoothly move current yaw/pitch towards target using exponential damping
            float lerpFactor = 1.0f - std::exp(-m_MouseSmoothSpeed * dt);
            m_Yaw   = glm::mix(m_Yaw,   m_TargetYaw,   lerpFactor);
            m_Pitch = glm::mix(m_Pitch, m_TargetPitch, lerpFactor);

            // Apply rotation (pitch, yaw, roll=0)
            RecalculateView();
        }

        void HandleMovement(float dt)
        {
            float speed = m_MoveSpeed * (m_KeysDown[static_cast<size_t>(KeyCode::LeftShift)] ? 4.0f : 1.0f);

            // Build orientation matrix matching the one used in the view calculation
            glm::mat4 orient = glm::yawPitchRoll(glm::radians(m_Yaw), glm::radians(m_Pitch), 0.0f);

            // Derive basis vectors from orientation (OpenGL convention: -Z forward)
            glm::vec3 forward = glm::normalize(glm::vec3(orient * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
            glm::vec3 right   = glm::normalize(glm::vec3(orient * glm::vec4(1.0f, 0.0f,  0.0f, 0.0f)));
            glm::vec3 up      = glm::normalize(glm::cross(right, forward));

            glm::vec3 position = m_Position;

            if (m_KeysDown[static_cast<size_t>(KeyCode::W)]) position += forward * speed * dt;
            if (m_KeysDown[static_cast<size_t>(KeyCode::S)]) position -= forward * speed * dt;
            if (m_KeysDown[static_cast<size_t>(KeyCode::A)]) position -= right   * speed * dt;
            if (m_KeysDown[static_cast<size_t>(KeyCode::D)]) position += right   * speed * dt;
            if (m_KeysDown[static_cast<size_t>(KeyCode::Q)]) position -= up      * speed * dt;
            if (m_KeysDown[static_cast<size_t>(KeyCode::E)]) position += up      * speed * dt;

            m_Position = position;
            RecalculateView();
        }

        // ------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------
        void RecalculateProjection()
        {
            if(m_ProjectionType == ProjectionType::Perspective)
            {
                m_Projection = glm::perspective(glm::radians(m_VerticalFOV), m_AspectRatio, m_NearClip, m_FarClip);
            }
            else // Orthographic
            {
                float orthoHalfHeight = m_OrthoSize * 0.5f;
                float orthoHalfWidth  = orthoHalfHeight * m_AspectRatio;

                m_Projection = glm::ortho(-orthoHalfWidth, orthoHalfWidth,
                                          -orthoHalfHeight, orthoHalfHeight,
                                          -m_FarClip, m_FarClip);
            }
        }

        void RecalculateView()
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
                                   glm::yawPitchRoll(glm::radians(m_Yaw), glm::radians(m_Pitch), 0.0f);
            m_View = glm::inverse(transform);
        }

        // ------------------------------------------------------------
        // Orthographic-specific helpers
        // ------------------------------------------------------------
        void HandleOrthoPan(float dt)
        {
            (void)dt; // currently unused – future smoothing
            if(!m_MouseButtonsDown[static_cast<size_t>(MouseCode::ButtonMiddle)]) { m_FirstMousePan = true; return; }

            bool shift = m_KeysDown[static_cast<size_t>(KeyCode::LeftShift)] || m_KeysDown[static_cast<size_t>(KeyCode::RightShift)];

            // Allow panning with middle mouse even without shift key
            // but for now, keep the original behavior requiring shift
            if(!shift) { m_FirstMousePan = true; return; }

            if(m_FirstMousePan)
            {
                m_FirstMousePan = false;
                return; // skip first frame to avoid jump when switching modes
            }

            float dx = m_MouseDX;
            float dy = m_MouseDY;

            // Convert pixel delta to world units based on ortho size & viewport
            float panSpeed = m_OrthoSize / 600.0f; // 600: arbitrary scale factor gives nice feel

            glm::mat4 orient = glm::yawPitchRoll(glm::radians(m_Yaw), glm::radians(m_Pitch), 0.0f);
            glm::vec3 right = glm::normalize(glm::vec3(orient * glm::vec4(1,0,0,0)));
            glm::vec3 up    = glm::normalize(glm::vec3(orient * glm::vec4(0,1,0,0)));

            m_Position -= right * dx * panSpeed;
            m_Position += up    * dy * panSpeed;
            RecalculateView();
        }

        void HandleOrthoZoom()
        {
            if(m_MouseScrollY != 0.0f)
            {
                float zoomFactor = 1.0f - m_MouseScrollY * 0.1f; // 10% per notch
                if(zoomFactor < 0.1f) zoomFactor = 0.1f;
                m_OrthoSize *= zoomFactor;
                if(m_OrthoSize < 0.01f) m_OrthoSize = 0.01f;
                RecalculateProjection();
            }
        }
        
        void HandlePerspectiveZoom()
        {
            if(m_MouseScrollY != 0.0f)
            {
                glm::mat4 orient = glm::yawPitchRoll(glm::radians(m_Yaw), glm::radians(m_Pitch), 0.0f);
                glm::vec3 forward = glm::normalize(glm::vec3(orient * glm::vec4(0,0,-1,0)));
                
                // Move forward/backward based on scroll direction
                float zoomSpeed = 2.0f;
                m_Position += forward * m_MouseScrollY * zoomSpeed;
                RecalculateView();
            }
        }

        // Zoom via Shift + Left-mouse drag (Blender dolly)
        void HandleDragZoom()
        {
            bool shift = m_KeysDown[static_cast<size_t>(KeyCode::LeftShift)] || m_KeysDown[static_cast<size_t>(KeyCode::RightShift)];
            if(!shift || !m_MouseButtonsDown[static_cast<size_t>(MouseCode::ButtonLeft)])
            {
                m_FirstMouseZoom = true;
                return;
            }

            if(m_FirstMouseZoom)
            {
                m_FirstMouseZoom = false;
                return; // skip first frame to avoid jump when switching modes
            }

            float dy = m_MouseDY;

            if(std::abs(dy) < 0.0001f) return;

            const float kZoomSpeed = 0.1f;

            if(m_ProjectionType == ProjectionType::Orthographic)
            {
                // Modify ortho size directly (dolly effect)
                float factor = 1.0f + dy * kZoomSpeed * 0.01f; // scale by small amount per pixel
                if(factor < 0.1f) factor = 0.1f;
                m_OrthoSize *= factor;
                if(m_OrthoSize < 0.01f) m_OrthoSize = 0.01f;
                RecalculateProjection();
            }
            else // perspective – move along forward vector
            {
                glm::mat4 orient = glm::yawPitchRoll(glm::radians(m_Yaw), glm::radians(m_Pitch), 0.0f);
                glm::vec3 forward = glm::normalize(glm::vec3(orient * glm::vec4(0,0,-1,0)));
                m_Position += forward * dy * kZoomSpeed;
                RecalculateView();
            }
        }

    private:
        // Projection
        ProjectionType m_ProjectionType { ProjectionType::Perspective };
        float m_VerticalFOV;
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;

        float m_OrthoSize { 20.0f }; // world units visible vertically in ortho

        // Transform state
        glm::vec3 m_Position {0.0f, 0.0f, 0.0f};

        // Euler angles
        float m_Yaw   = 0.0f;
        float m_Pitch = 0.0f;

        float m_TargetYaw   = 0.0f;
        float m_TargetPitch = 0.0f;

        // Matrices
        glm::mat4 m_View {1.0f};
        glm::mat4 m_Projection {1.0f};

        // Settings
        float m_MoveSpeed        = 2.0f;
        float m_MouseSensitivity = 0.1f;
        float m_MouseSmoothSpeed = 25.0f;

        // Cached for mouse delta
        bool  m_FirstMouse = true;

        // Pan state
        bool  m_FirstMousePan = true;

        // Drag zoom state
        bool  m_FirstMouseZoom = true;

        // Direct input state tracking (bypasses InputSystem)
        std::array<bool, 512> m_KeysDown{};
        std::array<bool, 8> m_MouseButtonsDown{};
        float m_MouseX = 0.0f;
        float m_MouseY = 0.0f;
        float m_MouseDX = 0.0f;
        float m_MouseDY = 0.0f;
        float m_MouseScrollX = 0.0f;
        float m_MouseScrollY = 0.0f;
        
        // Event subscription IDs
        std::vector<SubscriptionID> m_EventSubscriptions;

        // Cursor capture state while manipulating camera in viewport
        bool m_CursorCaptured { false };

        // Track last relative mode state to suppress first-delta jumps
        bool m_RelativeActiveLast { false };
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