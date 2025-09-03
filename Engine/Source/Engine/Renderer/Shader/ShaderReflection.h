#pragma once

#include "ShaderTypes.h"
#include "Core/Debug/ErrorCodes.h"

// Forward declarations for SPIRV-Cross
namespace spirv_cross 
{
    class Compiler;
    struct SPIRType;
    struct Resource;
}

namespace Vortex::Shader
{
    /**
     * @brief SPIR-V shader reflection system
     * 
     * Extracts comprehensive metadata from compiled SPIR-V bytecode including:
     * - Uniform buffers and their layouts
     * - Texture and sampler bindings
     * - Vertex input attributes
     * - Push constants
     * - Compute workgroup sizes
     * - Resource usage statistics
     */
    class ShaderReflection
    {
    public:
        ShaderReflection() = default;
        ~ShaderReflection() = default;

        /**
         * @brief Reflect a compiled SPIR-V shader
         * @param spirv SPIR-V bytecode
         * @param stage Shader stage for context
         * @return Reflection data or error
         */
        Result<ShaderReflectionData> Reflect(const std::vector<uint32_t>& spirv, ShaderStage stage);

        /**
         * @brief Combine reflection data from multiple shader stages
         * @param reflections Vector of reflection data from different stages
         * @return Combined reflection data
         */
        static ShaderReflectionData CombineReflections(const std::vector<ShaderReflectionData>& reflections);

        /**
         * @brief Validate shader compatibility for linking
         * @param vertexReflection Vertex shader reflection
         * @param fragmentReflection Fragment shader reflection
         * @return True if shaders are compatible for linking
         */
        static bool ValidateShaderCompatibility(const ShaderReflectionData& vertexReflection, 
                                               const ShaderReflectionData& fragmentReflection);

    private:
        // SPIRV-Cross helper methods
        ShaderDataType SPIRTypeToShaderDataType(const spirv_cross::SPIRType& type);
        ShaderResourceType DeduceResourceType(const spirv_cross::SPIRType& type);
        
        // Reflection extraction methods
        void ExtractUniforms(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection);
        void ExtractResources(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection);
        void ExtractVertexInputs(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection);
        void ExtractComputeInfo(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection);
        void ExtractStatistics(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection);
        
        // Utility methods
        uint32_t CalculateStructSize(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type);
        uint32_t CalculateAlignment(const spirv_cross::SPIRType& type);
    };

    /**
     * @brief Utility class for working with shader reflection data
     */
    class ReflectionUtils
    {
    public:
        /**
         * @brief Find a uniform by name in reflection data
         */
        static const ShaderUniform* FindUniform(const ShaderReflectionData& reflection, const std::string& name);
        
        /**
         * @brief Find a resource by name in reflection data
         */
        static const ShaderResource* FindResource(const ShaderReflectionData& reflection, const std::string& name);
        
        /**
         * @brief Get the total size of a uniform buffer
         */
        static uint32_t GetUniformBufferSize(const std::vector<ShaderUniform>& uniforms);
        
        /**
         * @brief Generate descriptor set layout info from reflection data
         */
        struct DescriptorSetLayout
        {
            uint32_t Set;
            std::vector<ShaderResource> Bindings;
        };
        static std::vector<DescriptorSetLayout> GenerateDescriptorSetLayouts(const ShaderReflectionData& reflection);
        
        /**
         * @brief Print detailed reflection information for debugging
         */
        static std::string PrintReflectionInfo(const ShaderReflectionData& reflection);
    };

} // namespace Vortex::Shader
