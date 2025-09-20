#version 450 core
// Per-vertex (static unit quad corners and uvs)
layout(location = 0) in vec2 aCorner;    // corners: (-1,-1),(1,-1),(1,1),(-1,1)
layout(location = 1) in vec2 aTexCoord;

// Per-instance attributes (divisor = 1)
layout(location = 2) in vec2 iCenter;
layout(location = 3) in vec2 iHalfSize;
layout(location = 4) in uint iColor;      // RGBA8 packed
layout(location = 5) in uint iTexIndex;   // texture index
layout(location = 6) in vec2 iRotSinCos;  // (cos(z), sin(z))
layout(location = 7) in float iZ;         // depth (world Z)

layout(location = 0) uniform mat4 u_ViewProjection;
// Engine-level pixel snapping controls (set by Renderer2D)
layout(location = 1) uniform vec2 u_ViewportSize;
layout(location = 2) uniform int  u_PixelSnap; // 0 = off, 1 = on

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) flat out int vTexIndex; // do not interpolate texture index

vec4 DecodeRGBA8(uint c)
{
    float r = float((c >> 24) & 0xFFu) / 255.0;
    float g = float((c >> 16) & 0xFFu) / 255.0;
    float b = float((c >> 8) & 0xFFu) / 255.0;
    float a = float(c & 0xFFu) / 255.0;
    return vec4(r, g, b, a);
}

void main()
{
    vColor = DecodeRGBA8(iColor);
    vTexCoord = aTexCoord;
    vTexIndex = int(iTexIndex);

    // Scale then rotate the corner
    vec2 local = aCorner * iHalfSize; // aCorner is in [-1,1]
    float c = iRotSinCos.x;
    float s = iRotSinCos.y;
    vec2 rotated = vec2(local.x * c - local.y * s, local.x * s + local.y * c);

    vec3 worldPos = vec3(iCenter + rotated, iZ);
    vec4 clip = u_ViewProjection * vec4(worldPos, 1.0);

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
