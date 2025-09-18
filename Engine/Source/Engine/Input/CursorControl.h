#pragma once

namespace Vortex
{
    // Platform cursor confinement/visibility control
    // Windows: implemented via ClipCursor; Other platforms: currently no-op
    struct CursorControl
    {
        // Confine cursor to a rectangle in screen coordinates [x0,y0,x1,y1]
        static void ConfineToRectScreen(int x0, int y0, int x1, int y1);
        // Release any confinement
        static void ReleaseConfine();
        // Hide/show cursor (optional)
        static void SetHidden(bool hidden);
    };
}
