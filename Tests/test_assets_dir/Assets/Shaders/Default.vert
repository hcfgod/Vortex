#version 450 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aUV;
            layout(location = 2) in vec3 aNormal;
            layout(location = 3) in vec3 aTangent;
            layout(location = 0) uniform mat4 u_ViewProjection;
            layout(location = 4) uniform mat4 u_Model;
            void main(){ gl_Position = u_ViewProjection * u_Model * vec4(aPos,1.0); }
            