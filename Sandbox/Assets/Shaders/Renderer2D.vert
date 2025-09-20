#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in float aTexIndex; // passed as float from VBO

layout(location = 0) uniform mat4 u_ViewProjection;
// Engine-level pixel snapping controls (set by Renderer2D)
layout(location = 1) uniform vec2 u_ViewportSize;
layout(location = 2) uniform int  u_PixelSnap; // 0 = off, 1 = on

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) flat out int vTexIndex; // do not interpolate texture index

void main()
{
    vColor = aColor;
    vTexCoord = aTexCoord;
    vTexIndex = int(aTexIndex + 0.5);

    vec4 clip = u_ViewProjection * vec4(aPos, 1.0);

    if (u_PixelSnap == 1 && u_ViewportSize.x > 0.0 && u_ViewportSize.y > 0.0)
    {
        // Convert to NDC
        vec2 ndc = clip.xy / clip.w;
        // NDC -> pixel coords
        vec2 pix = (ndc * 0.5 + 0.5) * u_ViewportSize;
        // Snap to pixel centers
        pix = floor(pix) + 0.5;
        // pixel -> NDC
        ndc = (pix / u_ViewportSize) * 2.0 - 1.0;
        // Back to clip space
        clip.xy = ndc * clip.w;
    }

    gl_Position = clip;
}
