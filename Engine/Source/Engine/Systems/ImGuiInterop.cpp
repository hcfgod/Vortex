#include "vxpch.h"
#include "ImGuiInterop.h"

namespace Vortex
{
    float ImGuiViewportInput::s_X0 = 0.0f;
    float ImGuiViewportInput::s_Y0 = 0.0f;
    float ImGuiViewportInput::s_X1 = 0.0f;
    float ImGuiViewportInput::s_Y1 = 0.0f;
    bool ImGuiViewportInput::s_Hovered = false;
    bool ImGuiViewportInput::s_Focused = false;

    void ImGuiViewportInput::SetViewportRect(float x0, float y0, float x1, float y1)
    {
        s_X0 = x0; s_Y0 = y0; s_X1 = x1; s_Y1 = y1;
    }

    bool ImGuiViewportInput::IsMouseInside(float x, float y)
    {
        return x >= s_X0 && x <= s_X1 && y >= s_Y0 && y <= s_Y1;
    }

    void ImGuiViewportInput::SetHoverFocus(bool hovered, bool focused)
    {
        s_Hovered = hovered; s_Focused = focused;
    }

    bool ImGuiViewportInput::IsFocused()
    {
        return s_Focused || s_Hovered;
    }

    bool ImGuiViewportInput::IsHovered()
    {
        return s_Hovered;
    }

    bool ImGuiViewportInput::HasKeyboardFocus()
    {
        return s_Focused;
    }
}


