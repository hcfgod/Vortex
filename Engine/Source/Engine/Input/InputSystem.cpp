#include "vxpch.h"
#include "InputSystem.h"
#include "Core/Debug/Log.h"
#include "Core/Events/EventSystem.h"
#include "InputActions.h"
#include "Engine/Systems/EngineSystem.h"
#include "Engine/Systems/SystemAccessors.h"

namespace Vortex
{
    InputSystem* InputSystem::s_Instance = nullptr;

    InputSystem::InputSystem()
        : EngineSystem("InputSystem", SystemPriority::High)
    {
    }

    Result<void> InputSystem::Initialize()
    {
        if (s_Instance)
        {
            VX_CORE_WARN("InputSystem already initialized");
            return Result<void>();
        }
        s_Instance = this;

        SubscribeEvents();
        MarkInitialized();
        VX_CORE_INFO("InputSystem initialized");
        return Result<void>();
    }

    Result<void> InputSystem::Update()
    {
        // Respect engine run mode: when in Edit mode, neutralize gameplay input state
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
            {
                ClearAllStates();
                return Result<void>();
            }
        }

        // Evaluate actions based on current device state (after events have been processed this frame)
        // Do NOT clear per-frame flags here; layers still need access to edge states during OnUpdate.
        EvaluateActions();
        return Result<void>();
    }

    Result<void> InputSystem::Shutdown()
    {
        UnsubscribeEvents();
        MarkShutdown();
        if (s_Instance == this) s_Instance = nullptr;
        VX_CORE_INFO("InputSystem shutdown complete");
        return Result<void>();
    }

    Result<void> InputSystem::Render()
    {
        // Clear per-frame flags and deltas at the end of the frame.
        // This preserves edge-triggered input for layer OnUpdate and action evaluation.
        ClearPerFrameFlags();
        return Result<void>();
    }

    // ============================ Queries ============================
    bool InputSystem::GetKey(KeyCode key) const
    {
        const size_t idx = KeyToIndex(key);
        return idx < KeyboardState::MaxKeys ? m_Keyboard.down[idx] : false;
    }

    bool InputSystem::GetKeyDown(KeyCode key) const
    {
        const size_t idx = KeyToIndex(key);
        return idx < KeyboardState::MaxKeys ? m_Keyboard.pressed[idx] : false;
    }

    bool InputSystem::GetKeyUp(KeyCode key) const
    {
        const size_t idx = KeyToIndex(key);
        return idx < KeyboardState::MaxKeys ? m_Keyboard.released[idx] : false;
        
    }

    bool InputSystem::GetMouseButton(MouseCode button) const
    {
        const size_t idx = MouseToIndex(button);
        return idx < MouseState::MaxButtons ? m_Mouse.down[idx] : false;
    }

    bool InputSystem::GetMouseButtonDown(MouseCode button) const
    {
        const size_t idx = MouseToIndex(button);
        return idx < MouseState::MaxButtons ? m_Mouse.pressed[idx] : false;
    }

    bool InputSystem::GetMouseButtonUp(MouseCode button) const
    {
        const size_t idx = MouseToIndex(button);
        return idx < MouseState::MaxButtons ? m_Mouse.released[idx] : false;
    }

    void InputSystem::GetMousePosition(float& outX, float& outY) const
    {
        outX = m_Mouse.x; outY = m_Mouse.y;
    }

    void InputSystem::GetMouseDelta(float& outDX, float& outDY) const
    {
        outDX = m_Mouse.dx; outDY = m_Mouse.dy;
    }

    void InputSystem::GetMouseScroll(float& outSX, float& outSY) const
    {
        outSX = m_Mouse.scrollX; outSY = m_Mouse.scrollY;
    }

    int InputSystem::GetFirstConnectedGamepadIndex() const
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return -1;
        }
        for (size_t i = 0; i < m_Gamepads.size(); ++i)
        {
            if (m_Gamepads[i].connected)
                return static_cast<int>(i);
        }
        return -1; // No connected gamepad found
    }

    bool InputSystem::IsGamepadConnected(int index) const
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false;
        }
        return index >= 0 && static_cast<size_t>(index) < m_Gamepads.size() && m_Gamepads[index].connected;
    }

    bool InputSystem::GetGamepadButton(int index, int button) const
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false;
        }
        if (!IsGamepadConnected(index)) return false;
        const auto& gp = m_Gamepads[index];
        return button >= 0 && button < static_cast<int>(GamepadState::MaxButtons) ? gp.buttons[button].down : false;
    }

    bool InputSystem::GetGamepadButtonDown(int index, int button) const
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false;
        }
        if (!IsGamepadConnected(index)) return false;
        const auto& gp = m_Gamepads[index];
        return button >= 0 && button < static_cast<int>(GamepadState::MaxButtons) ? gp.buttons[button].pressed : false;
    }

    bool InputSystem::GetGamepadButtonUp(int index, int button) const
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false;
        }
        if (!IsGamepadConnected(index)) return false;
        const auto& gp = m_Gamepads[index];
        return button >= 0 && button < static_cast<int>(GamepadState::MaxButtons) ? gp.buttons[button].released : false;
    }

    float InputSystem::GetGamepadAxis(int index, int axis) const
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return 0.0f;
        }
        if (!IsGamepadConnected(index)) return 0.0f;
        const auto& gp = m_Gamepads[index];
        return axis >= 0 && axis < static_cast<int>(GamepadState::MaxAxes) ? gp.axes[axis] : 0.0f;
    }

    // ============================ Action Maps ============================
    std::shared_ptr<InputActionMap> InputSystem::CreateActionMap(const std::string& name)
    {
        auto map = std::make_shared<InputActionMap>();
        m_ActionMaps[name] = map;
        return map;
    }

    void InputSystem::AddActionMap(const std::shared_ptr<InputActionMap>& map)
    {
        if (!map) return;
        // For now, use address as key if name not provided externally
        m_ActionMaps[fmt::format("map_{}", reinterpret_cast<uintptr_t>(map.get()))] = map;
    }

    bool InputSystem::RemoveActionMap(const std::string& name)
    {
        return m_ActionMaps.erase(name) > 0;
    }

    std::shared_ptr<InputActionMap> InputSystem::GetActionMap(const std::string& name)
    {
        auto it = m_ActionMaps.find(name);
        return it != m_ActionMaps.end() ? it->second : nullptr;
    }

    // ============================ Events ============================
    void InputSystem::SubscribeEvents()
    {
        m_Subscriptions.reserve(16);
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(KeyPressedEvent, this, &InputSystem::OnKeyPressed));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(KeyReleasedEvent, this, &InputSystem::OnKeyReleased));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseMovedEvent, this, &InputSystem::OnMouseMoved));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseScrolledEvent, this, &InputSystem::OnMouseScroll));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseButtonPressedEvent, this, &InputSystem::OnMouseButtonPressed));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(MouseButtonReleasedEvent, this, &InputSystem::OnMouseButtonReleased));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(GamepadConnectedEvent, this, &InputSystem::OnGamepadConnected));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(GamepadDisconnectedEvent, this, &InputSystem::OnGamepadDisconnected));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(GamepadButtonPressedEvent, this, &InputSystem::OnGamepadButtonDown));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(GamepadButtonReleasedEvent, this, &InputSystem::OnGamepadButtonUp));
        m_Subscriptions.push_back(VX_SUBSCRIBE_EVENT_METHOD(GamepadAxisEvent, this, &InputSystem::OnGamepadAxis));
    }

    void InputSystem::UnsubscribeEvents()
    {
        for (auto id : m_Subscriptions) { VX_UNSUBSCRIBE_EVENT(id); }
        m_Subscriptions.clear();
    }

    bool InputSystem::OnKeyPressed(const KeyPressedEvent& e)
    {
        const size_t idx = KeyToIndex(e.GetKeyCode());
        if (idx < KeyboardState::MaxKeys)
        {
            if (!m_Keyboard.down[idx])
            {
                m_Keyboard.pressed[idx] = true;
            }
            m_Keyboard.down[idx] = true;
        }
        return false;
    }

    bool InputSystem::OnKeyReleased(const KeyReleasedEvent& e)
    {
        const size_t idx = KeyToIndex(e.GetKeyCode());
        if (idx < KeyboardState::MaxKeys)
        {
            if (m_Keyboard.down[idx])
            {
                m_Keyboard.released[idx] = true;
            }
            m_Keyboard.down[idx] = false;
        }
        return false;
    }

    bool InputSystem::OnMouseMoved(const MouseMovedEvent& e)
    {
        // delta will be computed in Update via ClearPerFrameFlags + we can track here
        float newX = e.GetX();
        float newY = e.GetY();
        m_Mouse.dx += (newX - m_Mouse.x);
        m_Mouse.dy += (newY - m_Mouse.y);
        m_Mouse.x = newX;
        m_Mouse.y = newY;
        return false;
    }

    bool InputSystem::OnMouseScroll(const MouseScrolledEvent& e)
    {
        m_Mouse.scrollX += e.GetXOffset();
        m_Mouse.scrollY += e.GetYOffset();
        return false;
    }

    bool InputSystem::OnMouseButtonPressed(const MouseButtonPressedEvent& e)
    {
        const size_t idx = MouseToIndex(e.GetMouseButton());
        if (idx < MouseState::MaxButtons)
        {
            if (!m_Mouse.down[idx]) m_Mouse.pressed[idx] = true;
            m_Mouse.down[idx] = true;
        }
        return false;
    }

    bool InputSystem::OnMouseButtonReleased(const MouseButtonReleasedEvent& e)
    {
        const size_t idx = MouseToIndex(e.GetMouseButton());
        if (idx < MouseState::MaxButtons)
        {
            if (m_Mouse.down[idx]) m_Mouse.released[idx] = true;
            m_Mouse.down[idx] = false;
        }
        return false;
    }

    GamepadState* InputSystem::FindGamepadById(int sdlId)
    {
        for (auto& g : m_Gamepads) if (g.id == sdlId) return &g;
        return nullptr;
    }

    GamepadState* InputSystem::GetOrCreateGamepad(int sdlId)
    {
        if (auto* gp = FindGamepadById(sdlId)) return gp;
        // find free slot
        for (auto& g : m_Gamepads)
        {
            if (!g.connected)
            {
                g.connected = true; g.id = sdlId; return &g;
            }
        }
        // otherwise push new
        m_Gamepads.push_back({});
        auto& g = m_Gamepads.back();
        g.connected = true; g.id = sdlId;
        return &g;
    }

    bool InputSystem::OnGamepadConnected(const GamepadConnectedEvent& e)
    {
        auto* gp = GetOrCreateGamepad(e.GetGamepadId());
        if (gp) VX_CORE_INFO("Gamepad connected: id {}", e.GetGamepadId());
        return false;
    }

    bool InputSystem::OnGamepadDisconnected(const GamepadDisconnectedEvent& e)
    {
        if (auto* gp = FindGamepadById(e.GetGamepadId()))
        {
            *gp = GamepadState{}; // reset state
            VX_CORE_INFO("Gamepad disconnected: id {}", e.GetGamepadId());
        }
        return false;
    }

    bool InputSystem::OnGamepadButtonDown(const GamepadButtonPressedEvent& e)
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false; // ignore in Edit mode
        }
        if (auto* gp = GetOrCreateGamepad(e.GetGamepadId()))
        {
            const int btn = e.GetButton();
            if (btn >= 0 && btn < static_cast<int>(GamepadState::MaxButtons))
            {
                auto& bs = gp->buttons[btn];
                if (!bs.down) bs.pressed = true;
                bs.down = true;
            }
        }
        return false;
    }

    bool InputSystem::OnGamepadButtonUp(const GamepadButtonReleasedEvent& e)
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false; // ignore in Edit mode
        }
        if (auto* gp = GetOrCreateGamepad(e.GetGamepadId()))
        {
            const int btn = e.GetButton();
            if (btn >= 0 && btn < static_cast<int>(GamepadState::MaxButtons))
            {
                auto& bs = gp->buttons[btn];
                if (bs.down) bs.released = true;
                bs.down = false;
            }
        }
        return false;
    }

    bool InputSystem::OnGamepadAxis(const GamepadAxisEvent& e)
    {
        if (auto* eng = GetEngine())
        {
            if (!eng->IsGameplayInputEnabled())
                return false; // ignore in Edit mode
        }
        if (auto* gp = GetOrCreateGamepad(e.GetGamepadId()))
        {
            const int axis = e.GetAxis();
            if (axis >= 0 && axis < static_cast<int>(GamepadState::MaxAxes))
            {
                gp->axes[axis] = std::clamp(e.GetValue(), -1.0f, 1.0f);
            }
        }
        return false;
    }

    // ============================ Maintenance ============================
    void InputSystem::ClearPerFrameFlags()
    {
        // Keyboard edges
        for (size_t i = 0; i < KeyboardState::MaxKeys; ++i)
        {
            m_Keyboard.pressed[i] = false;
            m_Keyboard.released[i] = false;
        }
        // Mouse edges & deltas
        for (size_t i = 0; i < MouseState::MaxButtons; ++i)
        {
            m_Mouse.pressed[i] = false;
            m_Mouse.released[i] = false;
        }
        m_Mouse.dx = 0.0f; m_Mouse.dy = 0.0f;
        m_Mouse.scrollX = 0.0f; m_Mouse.scrollY = 0.0f;

        // Gamepads
        for (auto& g : m_Gamepads)
        {
            if (!g.connected) continue;
            for (auto& bs : g.buttons) { bs.pressed = false; bs.released = false; }
        }
    }

    void InputSystem::ClearAllStates()
    {
        // Reset keyboard
        for (size_t i = 0; i < KeyboardState::MaxKeys; ++i)
        {
            m_Keyboard.down[i] = false;
            m_Keyboard.pressed[i] = false;
            m_Keyboard.released[i] = false;
        }
        // Reset mouse
        for (size_t i = 0; i < MouseState::MaxButtons; ++i)
        {
            m_Mouse.down[i] = false;
            m_Mouse.pressed[i] = false;
            m_Mouse.released[i] = false;
        }
        m_Mouse.dx = 0.0f; m_Mouse.dy = 0.0f;
        m_Mouse.scrollX = 0.0f; m_Mouse.scrollY = 0.0f;

        // Reset gamepads
        for (auto& g : m_Gamepads)
        {
            for (auto& bs : g.buttons)
            {
                bs.down = bs.pressed = bs.released = false;
            }
            for (auto& ax : g.axes) ax = 0.0f;
        }
    }

    void InputSystem::EvaluateActions()
    {
        // First-pass: simple evaluation of actions
        for (auto& kv : m_ActionMaps)
        {
            auto& map = kv.second;
            if (!map) continue;
            map->Evaluate(this);
        }
    }
}