#include "vxpch.h"
#include "Camera.h"
#include "Core/Debug/Log.h"
#include "Engine/Systems/ImGuiInterop.h"

namespace Vortex
{
    void EditorCamera::SubscribeToInputEvents()
    {
        // Subscribe to input events directly
        m_EventSubscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(KeyPressedEvent, this, &EditorCamera::OnKeyPressed));
        m_EventSubscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(KeyReleasedEvent, this, &EditorCamera::OnKeyReleased));
        m_EventSubscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseMovedEvent, this, &EditorCamera::OnMouseMoved));
        m_EventSubscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseButtonPressedEvent, this, &EditorCamera::OnMouseButtonPressed));
        m_EventSubscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseButtonReleasedEvent, this, &EditorCamera::OnMouseButtonReleased));
        m_EventSubscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseScrolledEvent, this, &EditorCamera::OnMouseScrolled));
    }

    void EditorCamera::UnsubscribeFromInputEvents()
    {
        for (auto id : m_EventSubscriptions)
        {
            VX_UNSUBSCRIBE_EVENT(id);
        }
        m_EventSubscriptions.clear();
    }

    bool EditorCamera::OnKeyPressed(const KeyPressedEvent& event)
    {
        // Only handle input in Edit mode
        auto* app = Application::Get();
        if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
            return false;

        const size_t idx = static_cast<size_t>(event.GetKeyCode());
        if (idx < m_KeysDown.size())
        {
            m_KeysDown[idx] = true;
        }
        return false; // Don't consume the event
    }

    bool EditorCamera::OnKeyReleased(const KeyReleasedEvent& event)
    {
        // Only handle input in Edit mode
        auto* app = Application::Get();
        if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
            return false;

        const size_t idx = static_cast<size_t>(event.GetKeyCode());
        if (idx < m_KeysDown.size())
        {
            m_KeysDown[idx] = false;
        }
        return false; // Don't consume the event
    }

    bool EditorCamera::OnMouseMoved(const MouseMovedEvent& event)
    {
        // Only handle input in Edit mode
        auto* app = Application::Get();
        if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
            return false;

        // If not over viewport and not in relative mode, ignore move
        bool rel = app && app->IsRelativeMouseModeActive();
        if (!rel && !ImGuiViewportInput::IsHovered())
            return false;

        float newX = event.GetX();
        float newY = event.GetY();

        // When relative mode toggles on/off, suppress the first delta to avoid jump
        if (rel != m_RelativeActiveLast)
        {
            m_RelativeActiveLast = rel;
            m_MouseDX = 0.0f;
            m_MouseDY = 0.0f;
            m_MouseX = newX;
            m_MouseY = newY;
            return false;
        }
        
        // Only calculate delta if we have a previous position
        if (m_MouseX != 0.0f || m_MouseY != 0.0f)
        {
            m_MouseDX = newX - m_MouseX;
            m_MouseDY = newY - m_MouseY;
        }
        else
        {
            // First mouse position, no delta
            m_MouseDX = 0.0f;
            m_MouseDY = 0.0f;
        }
        
        m_MouseX = newX;
        m_MouseY = newY;
        
        return false; // Don't consume the event
    }

    bool EditorCamera::OnMouseButtonPressed(const MouseButtonPressedEvent& event)
    {
        // Only handle input in Edit mode
        auto* app = Application::Get();
        if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
            return false;

        // Only allow starting interactions when the viewport is hovered
        if (!ImGuiViewportInput::IsHovered())
            return false;

        const size_t idx = static_cast<size_t>(event.GetMouseButton());
        if (idx < m_MouseButtonsDown.size())
        {
            m_MouseButtonsDown[idx] = true;
        }
        return false; // Don't consume the event
    }

    bool EditorCamera::OnMouseButtonReleased(const MouseButtonReleasedEvent& event)
    {
        // Only handle input in Edit mode and when viewport is hovered
        auto* app = Application::Get();
        if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
            return false;
            
        // Always process button releases to avoid stuck buttons
        // even if mouse moved outside viewport

        const size_t idx = static_cast<size_t>(event.GetMouseButton());
        if (idx < m_MouseButtonsDown.size())
        {
            m_MouseButtonsDown[idx] = false;
        }
        return false; // Don't consume the event
    }

    bool EditorCamera::OnMouseScrolled(const MouseScrolledEvent& event)
    {
        // Only handle input in Edit mode
        auto* app = Application::Get();
        if (app && app->GetEngine() && app->GetEngine()->GetRunMode() != Engine::RunMode::Edit)
            return false;

        // Accept scroll if hovered or currently in relative mode (captured)
        if (!ImGuiViewportInput::IsHovered())
        {
            if (!(app && app->IsRelativeMouseModeActive()))
                return false;
        }

        m_MouseScrollX += event.GetXOffset();
        m_MouseScrollY += event.GetYOffset();
        return false; // Don't consume the event
    }
}
