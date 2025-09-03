#include "vxpch.h"
#include "ShaderTypes.h"

namespace Vortex
{
    const char* ShaderStageToString(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:                return "Vertex";
            case ShaderStage::TessellationControl:  return "TessellationControl";
            case ShaderStage::TessellationEvaluation: return "TessellationEvaluation";
            case ShaderStage::Geometry:             return "Geometry";
            case ShaderStage::Fragment:             return "Fragment";
            case ShaderStage::Compute:              return "Compute";
            case ShaderStage::RayGeneration:        return "RayGeneration";
            case ShaderStage::AnyHit:               return "AnyHit";
            case ShaderStage::ClosestHit:           return "ClosestHit";
            case ShaderStage::Miss:                 return "Miss";
            case ShaderStage::Intersection:         return "Intersection";
            case ShaderStage::Callable:             return "Callable";
            default:                                return "Unknown";
        }
    }
    
    const char* ShaderLanguageToString(ShaderLanguage language)
    {
        switch (language)
        {
            case ShaderLanguage::GLSL:  return "GLSL";
            case ShaderLanguage::HLSL:  return "HLSL";
            case ShaderLanguage::SPIRV: return "SPIR-V";
            default:                    return "Unknown";
        }
    }
    
    const char* ShaderDataTypeToString(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Bool:    return "bool";
            case ShaderDataType::Int:     return "int";
            case ShaderDataType::UInt:    return "uint";
            case ShaderDataType::Float:   return "float";
            case ShaderDataType::Double:  return "double";
            case ShaderDataType::Vec2:    return "vec2";
            case ShaderDataType::Vec3:    return "vec3";
            case ShaderDataType::Vec4:    return "vec4";
            case ShaderDataType::IVec2:   return "ivec2";
            case ShaderDataType::IVec3:   return "ivec3";
            case ShaderDataType::IVec4:   return "ivec4";
            case ShaderDataType::UVec2:   return "uvec2";
            case ShaderDataType::UVec3:   return "uvec3";
            case ShaderDataType::UVec4:   return "uvec4";
            case ShaderDataType::Mat2:    return "mat2";
            case ShaderDataType::Mat3:    return "mat3";
            case ShaderDataType::Mat4:    return "mat4";
            case ShaderDataType::Struct:  return "struct";
            default:                      return "unknown";
        }
    }
    
    uint32_t GetShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Bool:    return 1;
            case ShaderDataType::Int:     return 4;
            case ShaderDataType::UInt:    return 4;
            case ShaderDataType::Float:   return 4;
            case ShaderDataType::Double:  return 8;
            case ShaderDataType::Vec2:    return 4 * 2;
            case ShaderDataType::Vec3:    return 4 * 3;
            case ShaderDataType::Vec4:    return 4 * 4;
            case ShaderDataType::IVec2:   return 4 * 2;
            case ShaderDataType::IVec3:   return 4 * 3;
            case ShaderDataType::IVec4:   return 4 * 4;
            case ShaderDataType::UVec2:   return 4 * 2;
            case ShaderDataType::UVec3:   return 4 * 3;
            case ShaderDataType::UVec4:   return 4 * 4;
            case ShaderDataType::Mat2:    return 4 * 2 * 2;
            case ShaderDataType::Mat3:    return 4 * 3 * 3;
            case ShaderDataType::Mat4:    return 4 * 4 * 4;
            case ShaderDataType::Struct:  return 0; // Variable size
            default:                      return 0;
        }
    }
    
    std::string ShaderStageToFileExtension(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:                return ".vert";
            case ShaderStage::TessellationControl:  return ".tesc";
            case ShaderStage::TessellationEvaluation: return ".tese";
            case ShaderStage::Geometry:             return ".geom";
            case ShaderStage::Fragment:             return ".frag";
            case ShaderStage::Compute:              return ".comp";
            case ShaderStage::RayGeneration:        return ".rgen";
            case ShaderStage::AnyHit:               return ".rahit";
            case ShaderStage::ClosestHit:           return ".rchit";
            case ShaderStage::Miss:                 return ".rmiss";
            case ShaderStage::Intersection:         return ".rint";
            case ShaderStage::Callable:             return ".rcall";
            default:                                return ".unknown";
        }
    }
    
    ShaderStage FileExtensionToShaderStage(const std::string& extension)
    {
        if (extension == ".vert" || extension == ".vs")   return ShaderStage::Vertex;
        if (extension == ".tesc" || extension == ".tc")   return ShaderStage::TessellationControl;
        if (extension == ".tese" || extension == ".te")   return ShaderStage::TessellationEvaluation;
        if (extension == ".geom" || extension == ".gs")   return ShaderStage::Geometry;
        if (extension == ".frag" || extension == ".fs" || extension == ".ps") return ShaderStage::Fragment;
        if (extension == ".comp" || extension == ".cs")   return ShaderStage::Compute;
        if (extension == ".rgen")                         return ShaderStage::RayGeneration;
        if (extension == ".rahit")                        return ShaderStage::AnyHit;
        if (extension == ".rchit")                        return ShaderStage::ClosestHit;
        if (extension == ".rmiss")                        return ShaderStage::Miss;
        if (extension == ".rint")                         return ShaderStage::Intersection;
        if (extension == ".rcall")                        return ShaderStage::Callable;
        
        return ShaderStage::Vertex; // Default fallback
    }
    
} // namespace Vortex
