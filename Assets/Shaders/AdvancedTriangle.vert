#version 450 core

// Input vertex attributes
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;

// Output to fragment shader
layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec3 v_WorldPos;
layout(location = 3) out vec3 v_ViewPos;
layout(location = 4) out vec3 v_Tangent;
layout(location = 5) out vec3 v_Bitangent;

// Uniform matrices
layout(location = 0)  uniform mat4 u_ViewProjection; // uses 0..3
layout(location = 4)  uniform mat4 u_View;          // uses 4..7
layout(location = 8)  uniform mat4 u_Model;         // uses 8..11
layout(location = 12) uniform mat3 u_NormalMatrix;  // uses 12..14

// Animation/uniforms shared across stages
layout(location = 15) uniform float u_Time;
layout(location = 16) uniform vec3  u_CameraPos;

// Transform uniforms (vertex-only)
layout(location = 17) uniform vec3 u_Translation;
layout(location = 18) uniform vec3 u_Rotation;
layout(location = 19) uniform vec3 u_Scale;

// Wind animation parameters (vertex-only)
layout(location = 20) uniform float u_WindStrength;
layout(location = 21) uniform vec2  u_WindDirection;

void main()
{
    // Apply local transformations
    vec3 position = a_Position * u_Scale;
    
    // Apply wind effect (vertex animation)
    float windEffect = sin(u_Time * 2.0 + position.x * 0.5) * u_WindStrength;
    position.y += windEffect * a_TexCoord.y; // More wind effect at the top
    
    // Rotation matrices
    float cosX = cos(u_Rotation.x), sinX = sin(u_Rotation.x);
    float cosY = cos(u_Rotation.y), sinY = sin(u_Rotation.y);
    float cosZ = cos(u_Rotation.z), sinZ = sin(u_Rotation.z);
    
    mat3 rotX = mat3(1, 0, 0, 0, cosX, -sinX, 0, sinX, cosX);
    mat3 rotY = mat3(cosY, 0, sinY, 0, 1, 0, -sinY, 0, cosY);
    mat3 rotZ = mat3(cosZ, -sinZ, 0, sinZ, cosZ, 0, 0, 0, 1);
    
    mat3 rotation = rotZ * rotY * rotX;
    position = rotation * position + u_Translation;
    
    // Calculate world position
    vec4 worldPos = u_Model * vec4(position, 1.0);
    v_WorldPos = worldPos.xyz;
    
    // Calculate view position
    vec4 viewPos = u_View * worldPos;
    v_ViewPos = viewPos.xyz;
    
    // Pass through texture coordinates
    v_TexCoord = a_TexCoord;
    
    // Transform normal, tangent, and calculate bitangent
    v_Normal = normalize(u_NormalMatrix * (rotation * a_Normal));
    v_Tangent = normalize(u_NormalMatrix * (rotation * a_Tangent));
    v_Bitangent = cross(v_Normal, v_Tangent);
    
    // Final position transformation
    gl_Position = u_ViewProjection * worldPos;
}
