#include "vxpch.h"
#include "CursorControl.h"

#if defined(_WIN32)
    #define NOMINMAX
    #include <windows.h>
#endif

namespace Vortex
{
    void CursorControl::ConfineToRectScreen(int x0, int y0, int x1, int y1)
    {
    #if defined(_WIN32)
        RECT r{ x0, y0, x1, y1 };
        ::ClipCursor(&r);
    #else
        (void)x0; (void)y0; (void)x1; (void)y1;
    #endif
    }

    void CursorControl::ReleaseConfine()
    {
    #if defined(_WIN32)
        ::ClipCursor(nullptr);
    #endif
    }

    void CursorControl::SetHidden(bool hidden)
    {
    #if defined(_WIN32)
        // ShowCursor returns display count; calling until it reaches the desired state can be messy.
        // For our purposes a single call is typically sufficient.
        ::ShowCursor(hidden ? FALSE : TRUE);
    #else
        (void)hidden;
    #endif
    }
}
