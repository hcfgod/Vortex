#version 450 core
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) flat in int vTexIndex;

uniform sampler2D u_Textures[32];

void main()
{
    vec4 texColor = texture(u_Textures[vTexIndex], vTexCoord);
    FragColor = vColor * texColor;
}
