#version 450 core

// Input from vertex shader
layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec3 v_WorldPos;
layout(location = 3) in vec3 v_ViewPos;
layout(location = 4) in vec3 v_Tangent;
layout(location = 5) in vec3 v_Bitangent;

// Output
layout(location = 0) out vec4 FragColor;

// Animation and shared uniforms (must match vertex shader where shared)
layout(location = 15) uniform float u_Time;       // kept for compatibility (not used)
layout(location = 16) uniform vec3  u_CameraPos;  // matches VS

// Material uniforms (fragment-only)
layout(location = 22) uniform vec3  u_Albedo;
layout(location = 23) uniform float u_Metallic;
layout(location = 24) uniform float u_Roughness;
layout(location = 25) uniform float u_AO;
layout(location = 26) uniform vec3  u_Emission;
layout(location = 27) uniform float u_Alpha;

// Lighting uniforms (fragment-only)
layout(location = 28) uniform vec3  u_LightPosition;
layout(location = 29) uniform vec3  u_LightColor;
layout(location = 30) uniform float u_LightIntensity;

// Rim/fresnel effects (fragment-only)
layout(location = 31) uniform float u_RimPower;
layout(location = 32) uniform vec3  u_RimColor;
layout(location = 33) uniform float u_FresnelStrength;

// Texture samplers
layout(binding = 0) uniform sampler2D u_AlbedoTexture;
layout(binding = 1) uniform sampler2D u_NormalTexture;
layout(binding = 2) uniform sampler2D u_MetallicRoughnessTexture;
layout(binding = 3) uniform sampler2D u_EmissionTexture;
layout(binding = 4) uniform sampler2D u_AOTexture;
layout(binding = 5) uniform samplerCube u_EnvironmentMap;

// Texture flags
layout(location = 34) uniform int u_UseAlbedoTexture;
layout(location = 35) uniform int u_UseNormalTexture;
layout(location = 36) uniform int u_UseMetallicRoughnessTexture;
layout(location = 37) uniform int u_UseEmissionTexture;
layout(location = 38) uniform int u_UseAOTexture;

// Constants
const float PI = 3.14159265359;

// Distribution GGX/Trowbridge-Reitz Normal Distribution Function
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Geometry Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

// Geometry Smith
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel Schlick
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Normal mapping
vec3 GetNormalFromMap()
{
    if (u_UseNormalTexture == 0)
        return normalize(v_Normal);
    
    vec3 tangentNormal = texture(u_NormalTexture, v_TexCoord).xyz * 2.0 - 1.0;
    
    vec3 N = normalize(v_Normal);
    vec3 T = normalize(v_Tangent);
    vec3 B = normalize(v_Bitangent);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

void main()
{
    // Sample textures
    vec3 albedo = u_Albedo;
    if (u_UseAlbedoTexture == 1)
        albedo *= texture(u_AlbedoTexture, v_TexCoord).rgb;
    
    float metallic = u_Metallic;
    float roughness = u_Roughness;
    if (u_UseMetallicRoughnessTexture == 1)
    {
        vec3 metallicRoughness = texture(u_MetallicRoughnessTexture, v_TexCoord).rgb;
        metallic *= metallicRoughness.b; // Blue channel for metallic
        roughness *= metallicRoughness.g; // Green channel for roughness
    }
    
    float ao = u_AO;
    if (u_UseAOTexture == 1)
        ao *= texture(u_AOTexture, v_TexCoord).r;
    
    vec3 emission = u_Emission;
    if (u_UseEmissionTexture == 1)
        emission += texture(u_EmissionTexture, v_TexCoord).rgb;
    
    // Get normal from normal map or use vertex normal
    vec3 N = GetNormalFromMap();
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    // Light calculation
    vec3 L = normalize(u_LightPosition - v_WorldPos);
    vec3 H = normalize(V + L);
    float distance = length(u_LightPosition - v_WorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_LightColor * u_LightIntensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Environmental reflection (simplified)
    if (u_UseAOTexture == 1) // Reusing this flag for environment mapping demo
    {
        vec3 R = reflect(-V, N);
        vec3 envColor = texture(u_EnvironmentMap, R).rgb;
        vec3 F_env = FresnelSchlick(max(dot(N, V), 0.0), F0);
        ambient += envColor * F_env * 0.1;
    }
    
    vec3 color = ambient + Lo;
    
    // Rim lighting effect
    float rim = 1.0 - max(dot(V, N), 0.0);
    rim = pow(rim, u_RimPower);
    color += u_RimColor * rim * u_FresnelStrength;
    
    // Add emission
    color += emission;
    
    // Removed time-based color modulation for solid color output
    
    // HDR tonemapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, u_Alpha);
}
