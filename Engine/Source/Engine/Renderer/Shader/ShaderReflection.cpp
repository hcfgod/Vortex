#include "vxpch.h"
#include "ShaderReflection.h"

namespace Vortex::Shader
{
    Result<ShaderReflectionData> ShaderReflection::Reflect(const std::vector<uint32_t>& spirv, ShaderStage /*stage*/)
    {
        if (spirv.empty())
        {
            return Result<ShaderReflectionData>(ErrorCode::InvalidParameter, "Empty SPIR-V code");
        }

        ShaderReflectionData reflection;
        // Minimal stub: populate basic stats only
        reflection.InstructionCount = static_cast<uint32_t>(spirv.size());
        return Result<ShaderReflectionData>(std::move(reflection));
    }

    ShaderReflectionData ShaderReflection::CombineReflections(const std::vector<ShaderReflectionData>& reflections)
    {
        ShaderReflectionData combined;
        for (const auto& r : reflections)
        {
            combined.Uniforms.insert(combined.Uniforms.end(), r.Uniforms.begin(), r.Uniforms.end());
            combined.Resources.insert(combined.Resources.end(), r.Resources.begin(), r.Resources.end());
            combined.VertexInputs.insert(combined.VertexInputs.end(), r.VertexInputs.begin(), r.VertexInputs.end());
            for (const auto& [binding, uniforms] : r.UniformBuffers)
            {
                auto& dest = combined.UniformBuffers[binding];
                dest.insert(dest.end(), uniforms.begin(), uniforms.end());
            }
            combined.InstructionCount += r.InstructionCount;
            combined.MemoryUsage += r.MemoryUsage;
            combined.LocalSize = glm::uvec3(
                std::max(combined.LocalSize.x, r.LocalSize.x),
                std::max(combined.LocalSize.y, r.LocalSize.y),
                std::max(combined.LocalSize.z, r.LocalSize.z)
            );
        }
        return combined;
    }

    bool ShaderReflection::ValidateShaderCompatibility(const ShaderReflectionData& /*vertexReflection*/,
                                                       const ShaderReflectionData& /*fragmentReflection*/)
    {
        // Stub always returns true. Implement proper interface matching later.
        return true;
    }

} // namespace Vortex::Shader
