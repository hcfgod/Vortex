#pragma once

namespace Vortex
{
    // Simple global conduit to coordinate input forwarding between ImGui panels and Application
    struct ImGuiViewportInput
    {
        // Screen-space rectangle of the viewport image region in the main window
        static void SetViewportRect(float x0, float y0, float x1, float y1);
        static bool IsMouseInside(float x, float y);

        // Focus/hover state for keyboard capture decisions
        static void SetHoverFocus(bool hovered, bool focused);
        static bool IsFocused();

    private:
        static float s_X0, s_Y0, s_X1, s_Y1;
        static bool s_Hovered;
        static bool s_Focused;
    };
}


