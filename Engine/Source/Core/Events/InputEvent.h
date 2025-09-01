#pragma once

#include "Event.h"
#include "Engine/Input/KeyCodes.h"
#include "Engine/Input/MouseCodes.h"

namespace Vortex
{
    // =============================================================================
    // KEYBOARD EVENTS
    // =============================================================================

    /**
     * @brief Base class for keyboard events
     */
    class KeyEvent : public Event
    {
    public:
        /**
         * @brief Get the key code
         * @return Key code that triggered this event
         */
        inline KeyCode GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

    protected:
        KeyEvent(KeyCode keyCode)
            : m_KeyCode(keyCode) {}

        KeyCode m_KeyCode;
    };

    /**
     * @brief Event triggered when a key is pressed
     */
    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode keyCode, bool isRepeat = false)
            : KeyEvent(keyCode), m_IsRepeat(isRepeat) {}

        /**
         * @brief Check if this is a repeat press (held key)
         * @return True if this is a repeat press, false for initial press
         */
        inline bool IsRepeat() const { return m_IsRepeat; }

        std::string ToString() const override
        {
            return fmt::format("KeyPressedEvent: {} (repeat: {})", static_cast<int>(m_KeyCode), m_IsRepeat);
        }

        EVENT_CLASS_TYPE(KeyPressed)

    private:
        bool m_IsRepeat;
    };

    /**
     * @brief Event triggered when a key is released
     */
    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(KeyCode keyCode)
            : KeyEvent(keyCode) {}

        std::string ToString() const override
        {
            return fmt::format("KeyReleasedEvent: {}", static_cast<int>(m_KeyCode));
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    /**
     * @brief Event triggered when a character is typed (for text input)
     */
    class KeyTypedEvent : public Event
    {
    public:
        KeyTypedEvent(uint32_t character)
            : m_Character(character) {}

        /**
         * @brief Get the typed character
         * @return Unicode character code
         */
        inline uint32_t GetCharacter() const { return m_Character; }

        std::string ToString() const override
        {
            return fmt::format("KeyTypedEvent: {} ('{}')", m_Character, static_cast<char>(m_Character));
        }

        EVENT_CLASS_TYPE(KeyTyped)
        EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

    private:
        uint32_t m_Character;
    };

    // =============================================================================
    // MOUSE EVENTS
    // =============================================================================

    /**
     * @brief Base class for mouse button events
     */
    class MouseButtonEvent : public Event
    {
    public:
        /**
         * @brief Get the mouse button code
         * @return Mouse button that triggered this event
         */
        inline MouseCode GetMouseButton() const { return m_Button; }

        EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton)

    protected:
        MouseButtonEvent(MouseCode button)
            : m_Button(button) {}

        MouseCode m_Button;
    };

    /**
     * @brief Event triggered when a mouse button is pressed
     */
    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(MouseCode button)
            : MouseButtonEvent(button) {}

        std::string ToString() const override
        {
            return fmt::format("MouseButtonPressedEvent: {}", static_cast<int>(m_Button));
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    /**
     * @brief Event triggered when a mouse button is released
     */
    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(MouseCode button)
            : MouseButtonEvent(button) {}

        std::string ToString() const override
        {
            return fmt::format("MouseButtonReleasedEvent: {}", static_cast<int>(m_Button));
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };

    /**
     * @brief Event triggered when the mouse is moved
     */
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y)
            : m_MouseX(x), m_MouseY(y) {}

        /**
         * @brief Get the X coordinate of the mouse
         * @return X position in window coordinates
         */
        inline float GetX() const { return m_MouseX; }

        /**
         * @brief Get the Y coordinate of the mouse
         * @return Y position in window coordinates
         */
        inline float GetY() const { return m_MouseY; }

        std::string ToString() const override
        {
            return fmt::format("MouseMovedEvent: {}, {}", m_MouseX, m_MouseY);
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)

    private:
        float m_MouseX, m_MouseY;
    };

    /**
     * @brief Event triggered when the mouse wheel is scrolled
     */
    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float xOffset, float yOffset)
            : m_XOffset(xOffset), m_YOffset(yOffset) {}

        /**
         * @brief Get the horizontal scroll offset
         * @return X scroll offset
         */
        inline float GetXOffset() const { return m_XOffset; }

        /**
         * @brief Get the vertical scroll offset
         * @return Y scroll offset
         */
        inline float GetYOffset() const { return m_YOffset; }

        std::string ToString() const override
        {
            return fmt::format("MouseScrolledEvent: {}, {}", m_XOffset, m_YOffset);
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)

    private:
        float m_XOffset, m_YOffset;
    };

    // =============================================================================
    // GAMEPAD EVENTS
    // =============================================================================

    /**
     * @brief Event triggered when a gamepad is connected
     */
    class GamepadConnectedEvent : public Event
    {
    public:
        GamepadConnectedEvent(int gamepadId)
            : m_GamepadId(gamepadId) {}

        /**
         * @brief Get the gamepad ID
         * @return ID of the connected gamepad
         */
        inline int GetGamepadId() const { return m_GamepadId; }

        std::string ToString() const override
        {
            return fmt::format("GamepadConnectedEvent: ID {}", m_GamepadId);
        }

        EVENT_CLASS_TYPE(GamepadConnected)
        EVENT_CLASS_CATEGORY(EventCategory::Gamepad | EventCategory::Input)

    private:
        int m_GamepadId;
    };

    /**
     * @brief Event triggered when a gamepad is disconnected
     */
    class GamepadDisconnectedEvent : public Event
    {
    public:
        GamepadDisconnectedEvent(int gamepadId)
            : m_GamepadId(gamepadId) {}

        /**
         * @brief Get the gamepad ID
         * @return ID of the disconnected gamepad
         */
        inline int GetGamepadId() const { return m_GamepadId; }

        std::string ToString() const override
        {
            return fmt::format("GamepadDisconnectedEvent: ID {}", m_GamepadId);
        }

        EVENT_CLASS_TYPE(GamepadDisconnected)
        EVENT_CLASS_CATEGORY(EventCategory::Gamepad | EventCategory::Input)

    private:
        int m_GamepadId;
    };

    /**
     * @brief Event triggered when a gamepad button is pressed
     */
    class GamepadButtonPressedEvent : public Event
    {
    public:
        GamepadButtonPressedEvent(int gamepadId, int button)
            : m_GamepadId(gamepadId), m_Button(button) {}

        /**
         * @brief Get the gamepad ID
         * @return ID of the gamepad
         */
        inline int GetGamepadId() const { return m_GamepadId; }

        /**
         * @brief Get the button code
         * @return Button that was pressed
         */
        inline int GetButton() const { return m_Button; }

        std::string ToString() const override
        {
            return fmt::format("GamepadButtonPressedEvent: Gamepad {} Button {}", m_GamepadId, m_Button);
        }

        EVENT_CLASS_TYPE(GamepadButtonPressed)
        EVENT_CLASS_CATEGORY(EventCategory::Gamepad | EventCategory::Input)

    private:
        int m_GamepadId;
        int m_Button;
    };

    /**
     * @brief Event triggered when a gamepad button is released
     */
    class GamepadButtonReleasedEvent : public Event
    {
    public:
        GamepadButtonReleasedEvent(int gamepadId, int button)
            : m_GamepadId(gamepadId), m_Button(button) {}

        /**
         * @brief Get the gamepad ID
         * @return ID of the gamepad
         */
        inline int GetGamepadId() const { return m_GamepadId; }

        /**
         * @brief Get the button code
         * @return Button that was released
         */
        inline int GetButton() const { return m_Button; }

        std::string ToString() const override
        {
            return fmt::format("GamepadButtonReleasedEvent: Gamepad {} Button {}", m_GamepadId, m_Button);
        }

        EVENT_CLASS_TYPE(GamepadButtonReleased)
        EVENT_CLASS_CATEGORY(EventCategory::Gamepad | EventCategory::Input)

    private:
        int m_GamepadId;
        int m_Button;
    };

    /**
     * @brief Event triggered when a gamepad axis value changes
     */
    class GamepadAxisEvent : public Event
    {
    public:
        GamepadAxisEvent(int gamepadId, int axis, float value)
            : m_GamepadId(gamepadId), m_Axis(axis), m_Value(value) {}

        /**
         * @brief Get the gamepad ID
         * @return ID of the gamepad
         */
        inline int GetGamepadId() const { return m_GamepadId; }

        /**
         * @brief Get the axis code
         * @return Axis that changed
         */
        inline int GetAxis() const { return m_Axis; }

        /**
         * @brief Get the axis value
         * @return Current axis value (-1.0 to 1.0)
         */
        inline float GetValue() const { return m_Value; }

        std::string ToString() const override
        {
            return fmt::format("GamepadAxisEvent: Gamepad {} Axis {} Value {:.3f}", m_GamepadId, m_Axis, m_Value);
        }

        EVENT_CLASS_TYPE(GamepadAxis)
        EVENT_CLASS_CATEGORY(EventCategory::Gamepad | EventCategory::Input)

    private:
        int m_GamepadId;
        int m_Axis;
        float m_Value;
    };
}
