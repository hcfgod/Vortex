#pragma once

namespace Vortex
{
    /**
     * @brief Key codes based on SDL3 scancodes
     * 
     * These values correspond to physical keys on the keyboard and are layout-independent.
     * Based on the USB HID specification.
     */
    enum class KeyCode : uint16_t
    {
        // Printable keys
        Space         = 44,
        Apostrophe    = 52,  /* ' */
        Comma         = 54,  /* , */
        Minus         = 45,  /* - */
        Period        = 55,  /* . */
        Slash         = 56,  /* / */

        D0 = 39,  /* 0 */
        D1 = 30,  /* 1 */
        D2 = 31,  /* 2 */
        D3 = 32,  /* 3 */
        D4 = 33,  /* 4 */
        D5 = 34,  /* 5 */
        D6 = 35,  /* 6 */
        D7 = 36,  /* 7 */
        D8 = 37,  /* 8 */
        D9 = 38,  /* 9 */

        Semicolon     = 51,  /* ; */
        Equal         = 46,  /* = */

        A = 4,
        B = 5,
        C = 6,
        D = 7,
        E = 8,
        F = 9,
        G = 10,
        H = 11,
        I = 12,
        J = 13,
        K = 14,
        L = 15,
        M = 16,
        N = 17,
        O = 18,
        P = 19,
        Q = 20,
        R = 21,
        S = 22,
        T = 23,
        U = 24,
        V = 25,
        W = 26,
        X = 27,
        Y = 28,
        Z = 29,

        LeftBracket   = 47,  /* [ */
        Backslash     = 49,  /* \ */
        RightBracket  = 48,  /* ] */
        GraveAccent   = 53,  /* ` */

        World1        = 161, /* non-US #1 */
        World2        = 162, /* non-US #2 */

        // Function keys
        Escape        = 41,
        Enter         = 40,
        Tab           = 43,
        Backspace     = 42,
        Insert        = 73,
        Delete        = 76,
        Right         = 79,
        Left          = 80,
        Down          = 81,
        Up            = 82,
        PageUp        = 75,
        PageDown      = 78,
        Home          = 74,
        End           = 77,
        CapsLock      = 57,
        ScrollLock    = 71,
        NumLock       = 83,
        PrintScreen   = 70,
        Pause         = 72,

        F1  = 58,
        F2  = 59,
        F3  = 60,
        F4  = 61,
        F5  = 62,
        F6  = 63,
        F7  = 64,
        F8  = 65,
        F9  = 66,
        F10 = 67,
        F11 = 68,
        F12 = 69,
        F13 = 104,
        F14 = 105,
        F15 = 106,
        F16 = 107,
        F17 = 108,
        F18 = 109,
        F19 = 110,
        F20 = 111,
        F21 = 112,
        F22 = 113,
        F23 = 114,
        F24 = 115,

        // Keypad
        KP0        = 98,
        KP1        = 89,
        KP2        = 90,
        KP3        = 91,
        KP4        = 92,
        KP5        = 93,
        KP6        = 94,
        KP7        = 95,
        KP8        = 96,
        KP9        = 97,
        KPDecimal  = 99,
        KPDivide   = 84,
        KPMultiply = 85,
        KPSubtract = 86,
        KPAdd      = 87,
        KPEnter    = 88,
        KPEqual    = 103,

        // Modifier keys
        LeftShift    = 225,
        LeftControl  = 224,
        LeftAlt      = 226,
        LeftSuper    = 227,
        RightShift   = 229,
        RightControl = 228,
        RightAlt     = 230,
        RightSuper   = 231,
        Menu         = 101
    };

    /**
     * @brief Convert KeyCode to readable string
     * @param keyCode The key code to convert
     * @return String representation of the key
     */
    inline const char* KeyCodeToString(KeyCode keyCode)
    {
        switch (keyCode)
        {
            case KeyCode::Space:         return "Space";
            case KeyCode::Apostrophe:    return "Apostrophe";
            case KeyCode::Comma:         return "Comma";
            case KeyCode::Minus:         return "Minus";
            case KeyCode::Period:        return "Period";
            case KeyCode::Slash:         return "Slash";
            case KeyCode::D0:            return "0";
            case KeyCode::D1:            return "1";
            case KeyCode::D2:            return "2";
            case KeyCode::D3:            return "3";
            case KeyCode::D4:            return "4";
            case KeyCode::D5:            return "5";
            case KeyCode::D6:            return "6";
            case KeyCode::D7:            return "7";
            case KeyCode::D8:            return "8";
            case KeyCode::D9:            return "9";
            case KeyCode::Semicolon:     return "Semicolon";
            case KeyCode::Equal:         return "Equal";
            case KeyCode::A:             return "A";
            case KeyCode::B:             return "B";
            case KeyCode::C:             return "C";
            case KeyCode::D:             return "D";
            case KeyCode::E:             return "E";
            case KeyCode::F:             return "F";
            case KeyCode::G:             return "G";
            case KeyCode::H:             return "H";
            case KeyCode::I:             return "I";
            case KeyCode::J:             return "J";
            case KeyCode::K:             return "K";
            case KeyCode::L:             return "L";
            case KeyCode::M:             return "M";
            case KeyCode::N:             return "N";
            case KeyCode::O:             return "O";
            case KeyCode::P:             return "P";
            case KeyCode::Q:             return "Q";
            case KeyCode::R:             return "R";
            case KeyCode::S:             return "S";
            case KeyCode::T:             return "T";
            case KeyCode::U:             return "U";
            case KeyCode::V:             return "V";
            case KeyCode::W:             return "W";
            case KeyCode::X:             return "X";
            case KeyCode::Y:             return "Y";
            case KeyCode::Z:             return "Z";
            case KeyCode::LeftBracket:   return "LeftBracket";
            case KeyCode::Backslash:     return "Backslash";
            case KeyCode::RightBracket:  return "RightBracket";
            case KeyCode::GraveAccent:   return "GraveAccent";
            case KeyCode::World1:        return "World1";
            case KeyCode::World2:        return "World2";
            case KeyCode::Escape:        return "Escape";
            case KeyCode::Enter:         return "Enter";
            case KeyCode::Tab:           return "Tab";
            case KeyCode::Backspace:     return "Backspace";
            case KeyCode::Insert:        return "Insert";
            case KeyCode::Delete:        return "Delete";
            case KeyCode::Right:         return "Right";
            case KeyCode::Left:          return "Left";
            case KeyCode::Down:          return "Down";
            case KeyCode::Up:            return "Up";
            case KeyCode::PageUp:        return "PageUp";
            case KeyCode::PageDown:      return "PageDown";
            case KeyCode::Home:          return "Home";
            case KeyCode::End:           return "End";
            case KeyCode::CapsLock:      return "CapsLock";
            case KeyCode::ScrollLock:    return "ScrollLock";
            case KeyCode::NumLock:       return "NumLock";
            case KeyCode::PrintScreen:   return "PrintScreen";
            case KeyCode::Pause:         return "Pause";
            case KeyCode::F1:            return "F1";
            case KeyCode::F2:            return "F2";
            case KeyCode::F3:            return "F3";
            case KeyCode::F4:            return "F4";
            case KeyCode::F5:            return "F5";
            case KeyCode::F6:            return "F6";
            case KeyCode::F7:            return "F7";
            case KeyCode::F8:            return "F8";
            case KeyCode::F9:            return "F9";
            case KeyCode::F10:           return "F10";
            case KeyCode::F11:           return "F11";
            case KeyCode::F12:           return "F12";
            case KeyCode::F13:           return "F13";
            case KeyCode::F14:           return "F14";
            case KeyCode::F15:           return "F15";
            case KeyCode::F16:           return "F16";
            case KeyCode::F17:           return "F17";
            case KeyCode::F18:           return "F18";
            case KeyCode::F19:           return "F19";
            case KeyCode::F20:           return "F20";
            case KeyCode::F21:           return "F21";
            case KeyCode::F22:           return "F22";
            case KeyCode::F23:           return "F23";
            case KeyCode::F24:           return "F24";
            case KeyCode::KP0:           return "KP0";
            case KeyCode::KP1:           return "KP1";
            case KeyCode::KP2:           return "KP2";
            case KeyCode::KP3:           return "KP3";
            case KeyCode::KP4:           return "KP4";
            case KeyCode::KP5:           return "KP5";
            case KeyCode::KP6:           return "KP6";
            case KeyCode::KP7:           return "KP7";
            case KeyCode::KP8:           return "KP8";
            case KeyCode::KP9:           return "KP9";
            case KeyCode::KPDecimal:     return "KPDecimal";
            case KeyCode::KPDivide:      return "KPDivide";
            case KeyCode::KPMultiply:    return "KPMultiply";
            case KeyCode::KPSubtract:    return "KPSubtract";
            case KeyCode::KPAdd:         return "KPAdd";
            case KeyCode::KPEnter:       return "KPEnter";
            case KeyCode::KPEqual:       return "KPEqual";
            case KeyCode::LeftShift:     return "LeftShift";
            case KeyCode::LeftControl:   return "LeftControl";
            case KeyCode::LeftAlt:       return "LeftAlt";
            case KeyCode::LeftSuper:     return "LeftSuper";
            case KeyCode::RightShift:    return "RightShift";
            case KeyCode::RightControl:  return "RightControl";
            case KeyCode::RightAlt:      return "RightAlt";
            case KeyCode::RightSuper:    return "RightSuper";
            case KeyCode::Menu:          return "Menu";
            default:                     return "Unknown";
        }
    }
}
