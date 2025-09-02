#include "vxpch.h"
#include "InputActions.h"
#include "InputSystem.h"
#include "Engine/Input/Input.h"

namespace Vortex
{
    void InputAction::Evaluate(InputSystem* input)
    {
        if (!m_Enabled || !input) return;

        // Simple button logic: if any binding edge fires
        bool started = false;
        bool performed = false;
        bool canceled = false;

        for (const auto& b : m_Bindings)
        {
            switch (b.device)
            {
                case BindingDevice::Keyboard:
                {
                    if (input->GetKeyDown(b.key)) { started = true; performed = true; }
                    if (input->GetKeyUp(b.key)) { canceled = true; }
                    break;
                }
                case BindingDevice::Mouse:
                {
                    if (input->GetMouseButtonDown(b.mouse)) { started = true; performed = true; }
                    if (input->GetMouseButtonUp(b.mouse)) { canceled = true; }
                    break;
                }
                case BindingDevice::Gamepad:
                {
                    int idx = b.gamepadIndex;
                    if (idx < 0) {
                        idx = input->GetFirstConnectedGamepadIndex();
                        if (idx < 0) break; // no connected gamepad
                    }
                    if (b.gamepadButton >= 0)
                    {
                        if (input->GetGamepadButtonDown(idx, b.gamepadButton)) { started = true; performed = true; }
                        if (input->GetGamepadButtonUp(idx, b.gamepadButton)) { canceled = true; }
                    }
                    break;
                }
                default: break;
            }
        }

        if (started && m_OnStarted) Invoke(InputActionPhase::Started);
        if (performed && m_OnPerformed) Invoke(InputActionPhase::Performed);
        if (canceled && m_OnCanceled) Invoke(InputActionPhase::Canceled);

        if (canceled) m_Phase = InputActionPhase::Waiting;
    }

    void InputAction::Invoke(InputActionPhase phase)
    {
        switch (phase)
        {
            case InputActionPhase::Started: if (m_OnStarted) m_OnStarted(phase); break;
            case InputActionPhase::Performed: if (m_OnPerformed) m_OnPerformed(phase); break;
            case InputActionPhase::Canceled: if (m_OnCanceled) m_OnCanceled(phase); break;
            default: break;
        }
        m_Phase = phase;
    }
}

