#version 450 core

// Input vertex attributes
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

// Output to fragment shader
layout(location = 0) out vec2 v_TexCoord;

// Uniforms for transformation
layout(location = 0) uniform mat4 u_ViewProjection = mat4(1.0);
layout(location = 1) uniform mat4 u_Transform = mat4(1.0);

// Time-based uniforms for animation
layout(location = 2) uniform float u_Time = 0.0;

void main()
{
    v_TexCoord = a_TexCoord;
    
    // Apply basic transformation matrix
    vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
    
    // Add some simple rotation animation based on time
    float angle = u_Time * 0.5; // Rotate slowly
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    worldPos.xy = rotation * worldPos.xy;
    
    // Apply view-projection matrix
    gl_Position = u_ViewProjection * worldPos;
}
