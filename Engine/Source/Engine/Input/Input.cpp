#include "vxpch.h"
#include "Input.h"
#include "InputSystem.h"
#include "Core/Application.h"
#include "Engine/Engine.h"

namespace Vortex
{
    static InputSystem* S() { return InputSystem::Get(); }

    bool Input::GetKey(KeyCode key) { return S() ? S()->GetKey(key) : false; }
    bool Input::GetKeyDown(KeyCode key) { return S() ? S()->GetKeyDown(key) : false; }
    bool Input::GetKeyUp(KeyCode key) { return S() ? S()->GetKeyUp(key) : false; }

    bool Input::GetMouseButton(MouseCode button) { return S() ? S()->GetMouseButton(button) : false; }
    bool Input::GetMouseButtonDown(MouseCode button) { return S() ? S()->GetMouseButtonDown(button) : false; }
    bool Input::GetMouseButtonUp(MouseCode button) { return S() ? S()->GetMouseButtonUp(button) : false; }
    void Input::GetMousePosition(float& outX, float& outY) { if (S()) S()->GetMousePosition(outX, outY); else { outX = outY = 0.0f; } }
    void Input::GetMouseDelta(float& outDX, float& outDY) { if (S()) S()->GetMouseDelta(outDX, outDY); else { outDX = outDY = 0.0f; } }
    void Input::GetMouseScroll(float& outSX, float& outSY) { if (S()) S()->GetMouseScroll(outSX, outSY); else { outSX = outSY = 0.0f; } }

    static inline bool GameplayEnabled()
    {
        auto* app = Application::Get();
        auto* eng = app ? app->GetEngine() : nullptr;
        return eng && eng->IsGameplayInputEnabled();
    }

    int Input::GetFirstConnectedGamepadIndex() { return GameplayEnabled() && S() ? S()->GetFirstConnectedGamepadIndex() : -1; }
    bool Input::IsGamepadConnected(int index) { return GameplayEnabled() && S() ? S()->IsGamepadConnected(index) : false; }
    bool Input::GetGamepadButton(int index, int button) { return GameplayEnabled() && S() ? S()->GetGamepadButton(index, button) : false; }
    bool Input::GetGamepadButtonDown(int index, int button) { return GameplayEnabled() && S() ? S()->GetGamepadButtonDown(index, button) : false; }
    bool Input::GetGamepadButtonUp(int index, int button) { return GameplayEnabled() && S() ? S()->GetGamepadButtonUp(index, button) : false; }
    float Input::GetGamepadAxis(int index, int axis) { return GameplayEnabled() && S() ? S()->GetGamepadAxis(index, axis) : 0.0f; }
}