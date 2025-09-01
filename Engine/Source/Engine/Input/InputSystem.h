#pragma once

#include "vxpch.h"
#include "Engine/Systems/EngineSystem.h"
#include "Engine/Systems/SystemPriority.h"
#include "Core/Events/EventSystem.h"
#include "Core/Events/InputEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Engine/Input/KeyCodes.h"
#include "Engine/Input/MouseCodes.h"

namespace Vortex
{
    // Forward declarations
    class InputAction;
    class InputActionMap;

    // Device state structures
    struct KeyboardState
    {
        static constexpr size_t MaxKeys = 512;
        std::array<bool, MaxKeys> down{};
        std::array<bool, MaxKeys> pressed{};   // this frame
        std::array<bool, MaxKeys> released{};  // this frame
    };

    struct MouseState
    {
        float x = 0.0f;
        float y = 0.0f;
        float dx = 0.0f;   // delta since last frame
        float dy = 0.0f;
        float scrollX = 0.0f; // this frame
        float scrollY = 0.0f; // this frame

        static constexpr size_t MaxButtons = 8;
        std::array<bool, MaxButtons> down{};
        std::array<bool, MaxButtons> pressed{};   // this frame
        std::array<bool, MaxButtons> released{};  // this frame
    };

    struct GamepadButtonState
    {
        bool down = false;
        bool pressed = false;   // this frame
        bool released = false;  // this frame
    };

    struct GamepadState
    {
        bool connected = false;
        int id = -1; // SDL/gamepad id

        // Simple basic set; can be extended
        // 15 possible buttons for common pads
        static constexpr size_t MaxButtons = 32;
        std::array<GamepadButtonState, MaxButtons> buttons{};

        // Axes
        static constexpr size_t MaxAxes = 16;
        std::array<float, MaxAxes> axes{}; // -1..1
    };

    class InputSystem : public EngineSystem
    {
    public:
        InputSystem();
        ~InputSystem() override = default;

        // EngineSystem
        Result<void> Initialize() override;
        Result<void> Update() override;    // evaluate input actions (do not clear flags here)
        Result<void> Render() override;    // clear per-frame flags/deltas at end of frame
        Result<void> Shutdown() override;

        // Static access similar to EventSystem/Time
        static InputSystem* Get() { return s_Instance; }

        // Keyboard queries
        bool GetKey(KeyCode key) const;
        bool GetKeyDown(KeyCode key) const;
        bool GetKeyUp(KeyCode key) const;

        // Mouse queries
        bool GetMouseButton(MouseCode button) const;
        bool GetMouseButtonDown(MouseCode button) const;
        bool GetMouseButtonUp(MouseCode button) const;
        void GetMousePosition(float& outX, float& outY) const;
        void GetMouseDelta(float& outDX, float& outDY) const;
        void GetMouseScroll(float& outSX, float& outSY) const; // this frame

        // Gamepad (basic)
        size_t GetGamepadCount() const { return m_Gamepads.size(); }
        bool IsGamepadConnected(int index) const;
        bool GetGamepadButton(int index, int button) const;
        bool GetGamepadButtonDown(int index, int button) const;
        bool GetGamepadButtonUp(int index, int button) const;
        float GetGamepadAxis(int index, int axis) const;

        // Action system registration
        std::shared_ptr<InputActionMap> CreateActionMap(const std::string& name);
        void AddActionMap(const std::shared_ptr<InputActionMap>& map);
        bool RemoveActionMap(const std::string& name);
        std::shared_ptr<InputActionMap> GetActionMap(const std::string& name);

    private:
        // Event subscriptions
        void SubscribeEvents();
        void UnsubscribeEvents();

        // Event handlers (const event types per EventDispatcher)
        bool OnKeyPressed(const KeyPressedEvent& e);
        bool OnKeyReleased(const KeyReleasedEvent& e);
        bool OnMouseMoved(const MouseMovedEvent& e);
        bool OnMouseScroll(const MouseScrolledEvent& e);
        bool OnMouseButtonPressed(const MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(const MouseButtonReleasedEvent& e);
        bool OnGamepadConnected(const GamepadConnectedEvent& e);
        bool OnGamepadDisconnected(const GamepadDisconnectedEvent& e);
        bool OnGamepadButtonDown(const GamepadButtonPressedEvent& e);
        bool OnGamepadButtonUp(const GamepadButtonReleasedEvent& e);
        bool OnGamepadAxis(const GamepadAxisEvent& e);

        // Helpers
        size_t KeyToIndex(KeyCode key) const { return static_cast<size_t>(key); }
        size_t MouseToIndex(MouseCode btn) const { return static_cast<size_t>(btn); }
        GamepadState* FindGamepadById(int sdlId);
        GamepadState* GetOrCreateGamepad(int sdlId);

        // Per-frame maintenance
        void ClearPerFrameFlags();
        void EvaluateActions();

    private:
        static InputSystem* s_Instance;

        // Device states
        KeyboardState m_Keyboard{};
        MouseState m_Mouse{};
        std::vector<GamepadState> m_Gamepads; // keep a small vector

        // Event subscriptions for cleanup
        std::vector<SubscriptionID> m_Subscriptions;

        // Action system
        std::unordered_map<std::string, std::shared_ptr<InputActionMap>> m_ActionMaps;
    };
}