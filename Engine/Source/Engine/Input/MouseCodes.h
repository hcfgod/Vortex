#pragma once

namespace Vortex
{
    /**
     * @brief Mouse button codes based on SDL3 mouse buttons
     */
    enum class MouseCode : uint16_t
    {
        // Primary mouse buttons
        Button0 = 0,
        Button1 = 1,
        Button2 = 2,
        Button3 = 3,
        Button4 = 4,
        Button5 = 5,
        Button6 = 6,
        Button7 = 7,

        // Common aliases
        ButtonLeft   = Button0,
        ButtonRight  = Button1,
        ButtonMiddle = Button2,

        // Additional buttons (typically side buttons)
        ButtonX1 = Button3,
        ButtonX2 = Button4,

        // Maximum supported buttons
        ButtonLast = Button7
    };

    /**
     * @brief Convert MouseCode to readable string
     * @param mouseCode The mouse code to convert
     * @return String representation of the mouse button
     */
    inline const char* MouseCodeToString(MouseCode mouseCode)
    {
        switch (mouseCode)
        {
            case MouseCode::Button0:      return "Button0 (Left)";
            case MouseCode::Button1:      return "Button1 (Middle)";
            case MouseCode::Button2:      return "Button2 (Right)";
            case MouseCode::Button3:      return "Button3 (X1)";
            case MouseCode::Button4:      return "Button4 (X2)";
            case MouseCode::Button5:      return "Button5";
            case MouseCode::Button6:      return "Button6";
            case MouseCode::Button7:      return "Button7";
            default:                      return "Unknown";
        }
    }
}