#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord; // reserved, not used yet
layout(location = 3) in float aTexIndex; // reserved, not used yet

layout(location = 0) uniform mat4 u_ViewProjection;

out vec4 vColor;

void main()
{
    vColor = aColor;
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
}
