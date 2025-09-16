#version 450 core
            layout(location = 0) out vec4 FragColor;
            layout(location = 8) uniform vec4 u_Color;
            void main(){ FragColor = u_Color; }
        