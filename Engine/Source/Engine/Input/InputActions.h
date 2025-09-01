#pragma once

#include "vxpch.h"
#include "Engine/Input/KeyCodes.h"
#include "Engine/Input/MouseCodes.h"

namespace Vortex
{
    class InputSystem;

    enum class InputActionPhase : uint8_t
    {
        Waiting,
        Started,
        Performed,
        Canceled
    };

    enum class InputActionType : uint8_t
    {
        Button,
        Value,
        PassThrough
    };

    enum class BindingDevice : uint8_t
    {
        Keyboard,
        Mouse,
        Gamepad
    };

    struct InputBinding
    {
        BindingDevice device = BindingDevice::Keyboard;
        // For keyboard: key; for mouse: button; for gamepad: button/axis index
        KeyCode key = KeyCode{};
        MouseCode mouse = MouseCode{};
        int gamepadIndex = 0;
        int gamepadButton = -1;
        int gamepadAxis = -1;

        std::string path; // original string path

        static InputBinding KeyboardKey(KeyCode key, const std::string& pathHint = "")
        {
            InputBinding b{}; b.device = BindingDevice::Keyboard; b.key = key; b.path = pathHint; return b;
        }
        static InputBinding MouseButton(MouseCode btn, const std::string& pathHint = "")
        {
            InputBinding b{}; b.device = BindingDevice::Mouse; b.mouse = btn; b.path = pathHint; return b;
        }
    };

    class InputAction
    {
    public:
        using Callback = std::function<void(InputActionPhase)>;

        InputAction(const std::string& name, InputActionType type = InputActionType::Button)
            : m_Name(name), m_Type(type) {}

        const std::string& GetName() const { return m_Name; }
        InputActionType GetType() const { return m_Type; }

        void AddBinding(const InputBinding& binding) { m_Bindings.push_back(binding); }
        const std::vector<InputBinding>& GetBindings() const { return m_Bindings; }

        void Enable() { m_Enabled = true; m_Phase = InputActionPhase::Waiting; }
        void Disable() { m_Enabled = false; m_Phase = InputActionPhase::Waiting; }
        bool IsEnabled() const { return m_Enabled; }

        void SetCallbacks(Callback started, Callback performed, Callback canceled)
        {
            m_OnStarted = std::move(started);
            m_OnPerformed = std::move(performed);
            m_OnCanceled = std::move(canceled);
        }

        // Internal: Evaluated by InputSystem
        void Evaluate(InputSystem* input);

    private:
        void Invoke(InputActionPhase phase);

    private:
        std::string m_Name;
        InputActionType m_Type{ InputActionType::Button };
        std::vector<InputBinding> m_Bindings;
        bool m_Enabled{ true };
        InputActionPhase m_Phase{ InputActionPhase::Waiting };

        Callback m_OnStarted;
        Callback m_OnPerformed;
        Callback m_OnCanceled;
    };

    class InputActionMap
    {
    public:
        InputActionMap() = default;

        InputAction* CreateAction(const std::string& name, InputActionType type = InputActionType::Button)
        {
            auto action = std::make_unique<InputAction>(name, type);
            InputAction* ptr = action.get();
            m_Actions.emplace_back(std::move(action));
            return ptr;
        }

        void Enable() { m_Enabled = true; }
        void Disable() { m_Enabled = false; }
        bool IsEnabled() const { return m_Enabled; }

        void Evaluate(InputSystem* input)
        {
            if (!m_Enabled) return;
            for (auto& a : m_Actions) a->Evaluate(input);
        }

        const std::vector<std::unique_ptr<InputAction>>& GetActions() const { return m_Actions; }

    private:
        bool m_Enabled{ true };
        std::vector<std::unique_ptr<InputAction>> m_Actions;
    };
}

