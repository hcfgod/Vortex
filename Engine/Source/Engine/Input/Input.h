#pragma once

#include "Engine/Input/KeyCodes.h"
#include "Engine/Input/MouseCodes.h"

namespace Vortex
{
    // Static polling facade similar to Time
    class Input
    {
    public:
        Input() = delete;
        ~Input() = delete;

        // Keyboard
        static bool GetKey(KeyCode key);
        static bool GetKeyDown(KeyCode key);
        static bool GetKeyUp(KeyCode key);

        // Mouse
        static bool GetMouseButton(MouseCode button);
        static bool GetMouseButtonDown(MouseCode button);
        static bool GetMouseButtonUp(MouseCode button);
        static void GetMousePosition(float& outX, float& outY);
        static void GetMouseDelta(float& outDX, float& outDY);
        static void GetMouseScroll(float& outSX, float& outSY);

        // Gamepad basic
        static int GetFirstConnectedGamepadIndex(); // Returns -1 if none connected
        static bool IsGamepadConnected(int index);
        static bool GetGamepadButton(int index, int button);
        static bool GetGamepadButtonDown(int index, int button);
        static bool GetGamepadButtonUp(int index, int button);
        static float GetGamepadAxis(int index, int axis);
    };
}