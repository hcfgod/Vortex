#version 450 core

// Input from vertex shader
layout(location = 0) in vec2 v_TexCoord;

// Output color
layout(location = 0) out vec4 FragColor;

// Uniforms for color and animation
layout(location = 3) uniform vec3 u_Color = vec3(1.0, 0.5, 0.2); // Default orange
layout(location = 2) uniform float u_Time = 0.0;
layout(location = 4) uniform float u_Alpha = 1.0;

void main()
{
    // Create some animated effects
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(v_TexCoord, center);
    
    // Pulsing effect based on time
    float pulse = sin(u_Time * 2.0) * 0.5 + 0.5;
    
    // Color gradient based on texture coordinates
    vec3 gradient = mix(u_Color, u_Color * 1.5, v_TexCoord.x);
    
    // Add some variation based on distance from center
    gradient = mix(gradient, gradient * 0.7, dist);
    
    // Apply pulsing effect
    gradient = mix(gradient * 0.8, gradient, pulse);
    
    FragColor = vec4(gradient, u_Alpha);
}
