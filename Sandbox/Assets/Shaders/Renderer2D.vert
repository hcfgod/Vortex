#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in float aTexIndex; // passed as float from VBO

layout(location = 0) uniform mat4 u_ViewProjection;

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) flat out int vTexIndex; // do not interpolate texture index

void main()
{
    vColor = aColor;
    vTexCoord = aTexCoord;
    vTexIndex = int(aTexIndex + 0.5);
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
}
