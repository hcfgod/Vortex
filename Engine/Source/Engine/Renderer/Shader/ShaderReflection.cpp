#include "vxpch.h"
#include "ShaderReflection.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/ErrorCodes.h"

// Helper for unordered_map with pair keys
struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1); // Simple hash combination
    }
};

#ifdef VX_SPIRV_CROSS_AVAILABLE
    #include <spirv_cross/spirv_cross.hpp>
    #include <spirv_cross/spirv_glsl.hpp>
#endif

namespace Vortex
{
    Result<ShaderReflectionData> ShaderReflection::Reflect(const std::vector<uint32_t>& spirv, ShaderStage stage)
    {
        if (spirv.empty())
        {
            return Result<ShaderReflectionData>(ErrorCode::InvalidParameter, "Empty SPIR-V code");
        }

#ifdef VX_SPIRV_CROSS_AVAILABLE
        try
        {
            ShaderReflectionData reflection;
            
            // Create SPIRV-Cross compiler
            spirv_cross::Compiler compiler(spirv);
            
            // Extract all reflection data
            ExtractUniforms(compiler, reflection);
            ExtractResources(compiler, reflection);
            ExtractVertexInputs(compiler, reflection);
            
            if (stage == ShaderStage::Compute)
            {
                ExtractComputeInfo(compiler, reflection);
            }
            
            ExtractStatistics(compiler, reflection);
            
            return Result<ShaderReflectionData>(std::move(reflection));
        }
        catch (const spirv_cross::CompilerError& e)
        {
            VX_CORE_ERROR("SPIRV-Cross reflection failed: {}", e.what());
            return Result<ShaderReflectionData>(ErrorCode::ShaderCompilationFailed, e.what());
        }
        catch (const std::exception& e)
        {
            VX_CORE_ERROR("Shader reflection error: {}", e.what());
            return Result<ShaderReflectionData>(ErrorCode::Unknown, e.what());
        }
#else
        // Fallback: minimal reflection without SPIRV-Cross
        VX_CORE_WARN("SPIRV-Cross not available, using minimal reflection");
        ShaderReflectionData reflection;
        reflection.InstructionCount = static_cast<uint32_t>(spirv.size());
        return Result<ShaderReflectionData>(std::move(reflection));
#endif
    }

    ShaderReflectionData ShaderReflection::CombineReflections(const std::vector<ShaderReflectionData>& reflections)
    {
        ShaderReflectionData combined;

        // Maps to track merged resources by name for conflict resolution
        std::unordered_map<std::string, ShaderUniform> uniformMap;
        std::unordered_map<std::string, ShaderResource> resourceMap;
        std::unordered_map<std::string, ShaderVertexInput> vertexInputMap;
        std::unordered_map<uint32_t, std::vector<ShaderUniform>> uniformBufferMap;

        // Track which stages contributed to this reflection
        ShaderStageFlags contributingStages = 0;

        for (size_t stageIndex = 0; stageIndex < reflections.size(); ++stageIndex)
        {
            const auto& reflection = reflections[stageIndex];
            ShaderStageFlags currentStageFlag = 1u << stageIndex;

            // Merge uniforms with conflict resolution
            for (const auto& uniform : reflection.Uniforms)
            {
                auto it = uniformMap.find(uniform.Name);
                if (it != uniformMap.end())
                {
                    // Check for conflicts
                    if (!AreUniformsCompatible(it->second, uniform))
                    {
                        VX_CORE_ERROR("Uniform conflict detected for '{}': incompatible definitions across stages", uniform.Name);
                        // For now, keep the first definition, but this could be made configurable
                        continue;
                    }
                    // Merge stage information if needed
                }
                else
                {
                    uniformMap[uniform.Name] = uniform;
                }
            }

            // Merge resources with conflict resolution and stage tracking
            for (const auto& resource : reflection.Resources)
            {
                auto it = resourceMap.find(resource.Name);
                if (it != resourceMap.end())
                {
                    // Check for conflicts
                    if (!AreResourcesCompatible(it->second, resource))
                    {
                        VX_CORE_ERROR("Resource conflict detected for '{}': incompatible definitions across stages", resource.Name);
                        continue;
                    }
                    // Merge stages
                    it->second.Stages |= currentStageFlag;
                }
                else
                {
                    ShaderResource mergedResource = resource;
                    mergedResource.Stages = currentStageFlag;
                    resourceMap[resource.Name] = mergedResource;
                }
            }

            // Merge vertex inputs with conflict resolution
            for (const auto& vertexInput : reflection.VertexInputs)
            {
                auto it = vertexInputMap.find(vertexInput.Name);
                if (it != vertexInputMap.end())
                {
                    // Check for conflicts
                    if (!AreVertexInputsCompatible(it->second, vertexInput))
                    {
                        VX_CORE_ERROR("Vertex input conflict detected for '{}': incompatible definitions across stages", vertexInput.Name);
                        continue;
                    }
                }
                else
                {
                    vertexInputMap[vertexInput.Name] = vertexInput;
                }
            }

            // Merge uniform buffers with layout validation
            for (const auto& [binding, uniforms] : reflection.UniformBuffers)
            {
                auto it = uniformBufferMap.find(binding);
                if (it != uniformBufferMap.end())
                {
                    // Validate that the buffer layouts are compatible
                    if (!AreUniformBuffersCompatible(it->second, uniforms))
                    {
                        VX_CORE_ERROR("Uniform buffer conflict detected for binding {}: incompatible layouts across stages", binding);
                        continue;
                    }
                    // For now, keep the first definition. In practice, you might want to merge them
                }
                else
                {
                    uniformBufferMap[binding] = uniforms;
                }
            }

            // Accumulate statistics
            combined.InstructionCount += reflection.InstructionCount;
            combined.MemoryUsage = std::max(combined.MemoryUsage, reflection.MemoryUsage);

            // Handle compute workgroup size (take maximum across all stages)
            combined.LocalSize = glm::uvec3(
                std::max(combined.LocalSize.x, reflection.LocalSize.x),
                std::max(combined.LocalSize.y, reflection.LocalSize.y),
                std::max(combined.LocalSize.z, reflection.LocalSize.z)
            );

            contributingStages |= currentStageFlag;
        }

        // Convert maps back to vectors
        combined.Uniforms.reserve(uniformMap.size());
        for (const auto& [name, uniform] : uniformMap)
        {
            combined.Uniforms.push_back(uniform);
        }

        combined.Resources.reserve(resourceMap.size());
        for (const auto& [name, resource] : resourceMap)
        {
            combined.Resources.push_back(resource);
        }

        combined.VertexInputs.reserve(vertexInputMap.size());
        for (const auto& [name, vertexInput] : vertexInputMap)
        {
            combined.VertexInputs.push_back(vertexInput);
        }

        combined.UniformBuffers = std::move(uniformBufferMap);

        return combined;
    }

    bool ShaderReflection::ValidateShaderCompatibility(const ShaderReflectionData& vertexReflection,
                                                       const ShaderReflectionData& fragmentReflection)
    {
        bool isCompatible = true;

        // Validate vertex outputs against fragment inputs
        // Note: In a complete implementation, you'd need vertex output data from reflection
        // For now, we'll do basic structural validation

        // Check for uniform buffer compatibility
        for (const auto& [binding, vertexUniforms] : vertexReflection.UniformBuffers)
        {
            auto it = fragmentReflection.UniformBuffers.find(binding);
            if (it != fragmentReflection.UniformBuffers.end())
            {
                if (!AreUniformBuffersCompatible(vertexUniforms, it->second))
                {
                    VX_CORE_ERROR("Vertex and fragment shaders have incompatible uniform buffer at binding {}", binding);
                    isCompatible = false;
                }
            }
        }

        // Check for resource conflicts (same binding different types)
        std::unordered_map<std::pair<uint32_t, uint32_t>, ShaderResource, PairHash> vertexResources;
        std::unordered_map<std::pair<uint32_t, uint32_t>, ShaderResource, PairHash> fragmentResources;

        for (const auto& resource : vertexReflection.Resources)
        {
            vertexResources[{resource.Set, resource.Binding}] = resource;
        }

        for (const auto& resource : fragmentReflection.Resources)
        {
            fragmentResources[{resource.Set, resource.Binding}] = resource;
        }

        // Check for binding conflicts
        for (const auto& [key, vertexResource] : vertexResources)
        {
            auto it = fragmentResources.find(key);
            if (it != fragmentResources.end())
            {
                const auto& fragmentResource = it->second;
                if (vertexResource.Type != fragmentResource.Type)
                {
                    VX_CORE_ERROR("Resource binding conflict at set {}, binding {}: vertex uses {}, fragment uses {}",
                        vertexResource.Set, vertexResource.Binding,
                        static_cast<int>(vertexResource.Type), static_cast<int>(fragmentResource.Type));
                    isCompatible = false;
                }
            }
        }

        // Validate that vertex inputs are reasonable
        // (This is a basic check - more sophisticated validation would require output analysis)
        if (vertexReflection.VertexInputs.empty())
        {
            VX_CORE_WARN("Vertex shader has no vertex inputs - this may indicate an issue");
        }

        return isCompatible;
    }

    /**
     * @brief Validate the integrity of merged shader reflection data
     * @param reflection The merged reflection data to validate
     * @return True if the reflection data is internally consistent
     */
    bool ShaderReflection::ValidateMergedReflection(const ShaderReflectionData& reflection)
    {
        bool isValid = true;

        // Check for duplicate uniform names with different properties
        std::unordered_map<std::string, ShaderUniform> uniformMap;
        for (const auto& uniform : reflection.Uniforms)
        {
            auto it = uniformMap.find(uniform.Name);
            if (it != uniformMap.end())
            {
                if (!AreUniformsCompatible(it->second, uniform))
                {
                    VX_CORE_ERROR("Merged reflection contains incompatible uniforms with name '{}'", uniform.Name);
                    isValid = false;
                }
            }
            else
            {
                uniformMap[uniform.Name] = uniform;
            }
        }

        // Check for duplicate resource names with different properties
        std::unordered_map<std::string, ShaderResource> resourceMap;
        for (const auto& resource : reflection.Resources)
        {
            auto it = resourceMap.find(resource.Name);
            if (it != resourceMap.end())
            {
                if (!AreResourcesCompatible(it->second, resource))
                {
                    VX_CORE_ERROR("Merged reflection contains incompatible resources with name '{}'", resource.Name);
                    isValid = false;
                }
            }
            else
            {
                resourceMap[resource.Name] = resource;
            }
        }

        // Check for binding conflicts within the same descriptor set
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, ShaderResource>> setBindings;
        for (const auto& resource : reflection.Resources)
        {
            auto& bindings = setBindings[resource.Set];
            auto it = bindings.find(resource.Binding);
            if (it != bindings.end())
            {
                if (it->second.Type != resource.Type)
                {
                    VX_CORE_ERROR("Binding conflict in set {} at binding {}: {} vs {}",
                        resource.Set, resource.Binding,
                        static_cast<int>(it->second.Type), static_cast<int>(resource.Type));
                    isValid = false;
                }
            }
            else
            {
                bindings[resource.Binding] = resource;
            }
        }

        // Validate uniform buffer layouts
        for (const auto& [binding, uniforms] : reflection.UniformBuffers)
        {
            // Check for overlapping uniforms in the same buffer
            std::vector<std::pair<uint32_t, uint32_t>> ranges; // offset -> offset+size
            for (const auto& uniform : uniforms)
            {
                uint32_t start = uniform.Offset;
                uint32_t end = uniform.Offset + uniform.Size;
                for (const auto& range : ranges)
                {
                    if ((start >= range.first && start < range.second) ||
                        (end > range.first && end <= range.second) ||
                        (start <= range.first && end >= range.second))
                    {
                        VX_CORE_ERROR("Overlapping uniforms in buffer binding {}: offset collision", binding);
                        isValid = false;
                        break;
                    }
                }
                ranges.emplace_back(start, end);
            }
        }

        return isValid;
    }

    #ifdef VX_SPIRV_CROSS_AVAILABLE
    
    ShaderDataType ShaderReflection::SPIRTypeToShaderDataType(const spirv_cross::SPIRType& type)
    {
        using namespace spirv_cross;
        
        if (type.basetype == SPIRType::Boolean && type.vecsize == 1)
            return ShaderDataType::Bool;
        
        if (type.basetype == SPIRType::Int)
        {
            switch (type.vecsize)
            {
                case 1: return ShaderDataType::Int;
                case 2: return ShaderDataType::IVec2;
                case 3: return ShaderDataType::IVec3;
                case 4: return ShaderDataType::IVec4;
            }
        }
        
        if (type.basetype == SPIRType::UInt)
        {
            switch (type.vecsize)
            {
                case 1: return ShaderDataType::UInt;
                case 2: return ShaderDataType::UVec2;
                case 3: return ShaderDataType::UVec3;
                case 4: return ShaderDataType::UVec4;
            }
        }
        
        if (type.basetype == SPIRType::Float)
        {
            if (type.columns == 1)
            {
                switch (type.vecsize)
                {
                    case 1: return ShaderDataType::Float;
                    case 2: return ShaderDataType::Vec2;
                    case 3: return ShaderDataType::Vec3;
                    case 4: return ShaderDataType::Vec4;
                }
            }
            else if (type.columns > 1)  // Matrix types
            {
                if (type.columns == 2 && type.vecsize == 2)
                    return ShaderDataType::Mat2;
                if (type.columns == 3 && type.vecsize == 3)
                    return ShaderDataType::Mat3;
                if (type.columns == 4 && type.vecsize == 4)
                    return ShaderDataType::Mat4;
            }
        }
        
        if (type.basetype == SPIRType::Double)
            return ShaderDataType::Double;
        
        if (type.basetype == SPIRType::Struct)
            return ShaderDataType::Struct;
        
        return ShaderDataType::Unknown;
    }
    
    ShaderResourceType ShaderReflection::DeduceResourceType(const spirv_cross::SPIRType& type)
    {
        using namespace spirv_cross;
        
        switch (type.basetype)
        {
            case SPIRType::Image:
                switch (type.image.dim)
                {
                    case spv::Dim1D: return ShaderResourceType::Texture1D;
                    case spv::Dim2D: return ShaderResourceType::Texture2D;
                    case spv::Dim3D: return ShaderResourceType::Texture3D;
                    case spv::DimCube: return ShaderResourceType::TextureCube;
                    default: return ShaderResourceType::Texture2D;
                }
            case SPIRType::SampledImage:
                switch (type.image.dim)
                {
                    case spv::Dim1D: return ShaderResourceType::Texture1D;
                    case spv::Dim2D: return ShaderResourceType::Texture2D;
                    case spv::Dim3D: return ShaderResourceType::Texture3D;
                    case spv::DimCube: return ShaderResourceType::TextureCube;
                    default: return ShaderResourceType::Texture2D;
                }
            case SPIRType::Sampler:
                return ShaderResourceType::Sampler;
            case SPIRType::Struct:
                return ShaderResourceType::UniformBuffer;
            default:
                return ShaderResourceType::UniformBuffer;
        }
    }
    
    void ShaderReflection::ExtractUniforms(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection)
    {
        // Get all shader resources
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        
        // Extract uniform buffers
        for (const auto& resource : resources.uniform_buffers)
        {
            const auto& type = compiler.get_type(resource.type_id);
            uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            
            std::vector<ShaderUniform> bufferUniforms;
            
            // Process struct members
            for (uint32_t i = 0; i < type.member_types.size(); ++i)
            {
                const auto& memberType = compiler.get_type(type.member_types[i]);
                std::string memberName = compiler.get_member_name(resource.type_id, i);
                
                ShaderUniform uniform;
                uniform.Name = memberName;
                uniform.Type = SPIRTypeToShaderDataType(memberType);
                uniform.Size = static_cast<uint32_t>(compiler.get_declared_struct_member_size(type, i));
                uniform.Offset = compiler.type_struct_member_offset(type, i);
                uniform.ArraySize = memberType.array.empty() ? 0 : memberType.array[0];
                uniform.Location = binding;
                
                bufferUniforms.push_back(uniform);
                reflection.Uniforms.push_back(uniform);
            }
            
            reflection.UniformBuffers[binding] = bufferUniforms;
        }
        
        // Extract push constants
        for (const auto& resource : resources.push_constant_buffers)
        {
            const auto& type = compiler.get_type(resource.type_id);
            
            for (uint32_t i = 0; i < type.member_types.size(); ++i)
            {
                const auto& memberType = compiler.get_type(type.member_types[i]);
                std::string memberName = compiler.get_member_name(resource.type_id, i);
                
                ShaderUniform uniform;
                uniform.Name = memberName;
                uniform.Type = SPIRTypeToShaderDataType(memberType);
                uniform.Size = static_cast<uint32_t>(compiler.get_declared_struct_member_size(type, i));
                uniform.Offset = compiler.type_struct_member_offset(type, i);
                uniform.ArraySize = memberType.array.empty() ? 0 : memberType.array[0];
                uniform.Location = 0;  // Push constants don't have bindings
                
                reflection.Uniforms.push_back(uniform);
            }
        }
    }
    
    void ShaderReflection::ExtractResources(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        
        // Extract sampled images (textures)
        for (const auto& resource : resources.sampled_images)
        {
            const auto& type = compiler.get_type(resource.type_id);
            
            ShaderResource shaderResource;
            shaderResource.Name = resource.name;
            shaderResource.Type = DeduceResourceType(type);
            shaderResource.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            shaderResource.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            shaderResource.ArraySize = type.array.empty() ? 1 : type.array[0];
            shaderResource.Stages = 0;  // Will be set when combining reflections
            
            reflection.Resources.push_back(shaderResource);
        }
        
        // Extract separate images
        for (const auto& resource : resources.separate_images)
        {
            const auto& type = compiler.get_type(resource.type_id);
            
            ShaderResource shaderResource;
            shaderResource.Name = resource.name;
            shaderResource.Type = DeduceResourceType(type);
            shaderResource.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            shaderResource.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            shaderResource.ArraySize = type.array.empty() ? 1 : type.array[0];
            shaderResource.Stages = 0;
            
            reflection.Resources.push_back(shaderResource);
        }
        
        // Extract separate samplers
        for (const auto& resource : resources.separate_samplers)
        {
            const auto& type = compiler.get_type(resource.type_id);
            
            ShaderResource shaderResource;
            shaderResource.Name = resource.name;
            shaderResource.Type = ShaderResourceType::Sampler;
            shaderResource.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            shaderResource.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            shaderResource.ArraySize = type.array.empty() ? 1 : type.array[0];
            shaderResource.Stages = 0;
            
            reflection.Resources.push_back(shaderResource);
        }
        
        // Extract storage buffers
        for (const auto& resource : resources.storage_buffers)
        {
            const auto& type = compiler.get_type(resource.type_id);
            
            ShaderResource shaderResource;
            shaderResource.Name = resource.name;
            shaderResource.Type = ShaderResourceType::StorageBuffer;
            shaderResource.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            shaderResource.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            shaderResource.ArraySize = type.array.empty() ? 1 : type.array[0];
            shaderResource.Stages = 0;
            
            reflection.Resources.push_back(shaderResource);
        }
    }
    
    void ShaderReflection::ExtractVertexInputs(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        
        for (const auto& input : resources.stage_inputs)
        {
            const auto& type = compiler.get_type(input.type_id);
            
            ShaderVertexInput vertexInput;
            vertexInput.Name = input.name;
            vertexInput.Location = compiler.get_decoration(input.id, spv::DecorationLocation);
            vertexInput.Type = SPIRTypeToShaderDataType(type);
            vertexInput.Size = GetShaderDataTypeSize(vertexInput.Type);
            
            reflection.VertexInputs.push_back(vertexInput);
        }
    }
    
    void ShaderReflection::ExtractComputeInfo(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection)
    {
        // Get compute workgroup size
        const auto& execution = compiler.get_execution_model();
        if (execution == spv::ExecutionModelGLCompute)
        {
            // Use default workgroup size from entry points
            try {
                const auto& entryPoints = compiler.get_entry_points_and_stages();
                if (!entryPoints.empty())
                {
                    auto localSize = compiler.get_entry_point(entryPoints[0].name, entryPoints[0].execution_model).workgroup_size;
                    reflection.LocalSize = glm::uvec3(localSize.x, localSize.y, localSize.z);
                }
            } catch (...) {
                // Default to 1x1x1 workgroup if we can't extract the info
                reflection.LocalSize = glm::uvec3(1, 1, 1);
                VX_CORE_WARN("Failed to extract compute workgroup size, defaulting to (1,1,1)");
            }
        }
    }
    
    void ShaderReflection::ExtractStatistics(spirv_cross::Compiler& compiler, ShaderReflectionData& reflection)
    {
        // Get basic statistics - use a simple estimation since get_ir() API has changed
        try {
            // Estimate instruction count based on available data
            reflection.InstructionCount = 0;
            
            // Count resources as a rough measure of complexity
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            reflection.InstructionCount += static_cast<uint32_t>(
                resources.uniform_buffers.size() + 
                resources.sampled_images.size() + 
                resources.storage_buffers.size() + 
                resources.stage_inputs.size() + 
                resources.stage_outputs.size());
        } catch (...) {
            reflection.InstructionCount = 0;
        }
        
        // Estimate memory usage based on resources
        reflection.MemoryUsage = 0;
        for (const auto& uniform : reflection.Uniforms)
        {
            reflection.MemoryUsage += uniform.Size;
        }
    }
    
    uint32_t ShaderReflection::CalculateStructSize(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type)
    {
        return static_cast<uint32_t>(compiler.get_declared_struct_size(type));
    }
    
    uint32_t ShaderReflection::CalculateAlignment(const spirv_cross::SPIRType& type)
    {
        // Basic alignment calculation - can be enhanced
        switch (type.basetype)
        {
            case spirv_cross::SPIRType::Float:
            case spirv_cross::SPIRType::Int:
            case spirv_cross::SPIRType::UInt:
                return 4 * type.vecsize;
            case spirv_cross::SPIRType::Double:
                return 8 * type.vecsize;
            default:
                return 4;
        }
    }
    
    #endif // VX_SPIRV_CROSS_AVAILABLE

    // ============================================================================
    // Merge Validation Helper Methods
    // ============================================================================

    bool ShaderReflection::AreUniformsCompatible(const ShaderUniform& a, const ShaderUniform& b)
    {
        // Uniforms must have the same type and size
        if (a.Type != b.Type || a.Size != b.Size)
            return false;

        // Array sizes must match (0 means not an array)
        if (a.ArraySize != b.ArraySize)
            return false;

        // For uniforms in the same buffer, offsets should be consistent
        // But we allow different locations for cross-stage uniforms
        return true;
    }

    bool ShaderReflection::AreResourcesCompatible(const ShaderResource& a, const ShaderResource& b)
    {
        // Resources must have the same type
        if (a.Type != b.Type)
            return false;

        // Must be in the same descriptor set
        if (a.Set != b.Set)
            return false;

        // Must have the same binding point
        if (a.Binding != b.Binding)
            return false;

        // Array sizes must match
        if (a.ArraySize != b.ArraySize)
            return false;

        return true;
    }

    bool ShaderReflection::AreVertexInputsCompatible(const ShaderVertexInput& a, const ShaderVertexInput& b)
    {
        // Vertex inputs must have the same type and size
        if (a.Type != b.Type || a.Size != b.Size)
            return false;

        // Location must match for vertex inputs (this is critical for linking)
        if (a.Location != b.Location)
            return false;

        return true;
    }

    bool ShaderReflection::AreUniformBuffersCompatible(const std::vector<ShaderUniform>& a, const std::vector<ShaderUniform>& b)
    {
        // If one buffer is empty, consider them compatible (will use the non-empty one)
        if (a.empty() || b.empty())
            return true;

        // Buffers must have the same number of uniforms
        if (a.size() != b.size())
            return false;

        // Check that each uniform in buffer A has a corresponding uniform in buffer B
        // with the same name, type, and layout
        for (const auto& uniformA : a)
        {
            bool found = false;
            for (const auto& uniformB : b)
            {
                if (uniformA.Name == uniformB.Name)
                {
                    if (!AreUniformsCompatible(uniformA, uniformB))
                        return false;

                    // For uniform buffers, offsets must match exactly
                    if (uniformA.Offset != uniformB.Offset)
                        return false;

                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }

        return true;
    }

    // ============================================================================
    // ReflectionUtils Implementation
    // ============================================================================
    
    const ShaderUniform* ReflectionUtils::FindUniform(const ShaderReflectionData& reflection, const std::string& name)
    {
        for (const auto& uniform : reflection.Uniforms)
        {
            if (uniform.Name == name)
                return &uniform;
        }
        return nullptr;
    }
    
    const ShaderResource* ReflectionUtils::FindShaderResource(const ShaderReflectionData& reflection, const std::string& name)
    {
        for (const auto& resource : reflection.Resources)
        {
            if (resource.Name == name)
                return &resource;
        }
        return nullptr;
    }
    
    uint32_t ReflectionUtils::GetUniformBufferSize(const std::vector<ShaderUniform>& uniforms)
    {
        uint32_t totalSize = 0;
        for (const auto& uniform : uniforms)
        {
            totalSize = std::max(totalSize, uniform.Offset + uniform.Size);
        }
        return totalSize;
    }
    
    std::vector<ReflectionUtils::DescriptorSetLayout> ReflectionUtils::GenerateDescriptorSetLayouts(const ShaderReflectionData& reflection)
    {
        std::unordered_map<uint32_t, std::vector<ShaderResource>> setMap;
        
        // Group resources by descriptor set
        for (const auto& resource : reflection.Resources)
        {
            setMap[resource.Set].push_back(resource);
        }
        
        std::vector<DescriptorSetLayout> layouts;
        for (const auto& [set, resources] : setMap)
        {
            DescriptorSetLayout layout;
            layout.Set = set;
            layout.Bindings = resources;
            layouts.push_back(layout);
        }
        
        return layouts;
    }
    
    std::string ReflectionUtils::PrintReflectionInfo(const ShaderReflectionData& reflection)
    {
        std::ostringstream oss;
        
        oss << "Shader Reflection Information:\n";
        oss << "============================\n";
        oss << "Instructions: " << reflection.InstructionCount << "\n";
        oss << "Memory Usage: " << reflection.MemoryUsage << " bytes\n";
        
        if (reflection.LocalSize.x > 0 || reflection.LocalSize.y > 0 || reflection.LocalSize.z > 0)
        {
            oss << "Compute Workgroup Size: (" << reflection.LocalSize.x << ", " 
                << reflection.LocalSize.y << ", " << reflection.LocalSize.z << ")\n";
        }
        
        oss << "\nUniforms (" << reflection.Uniforms.size() << "):\n";
        for (const auto& uniform : reflection.Uniforms)
        {
            oss << "  - " << uniform.Name << " (" << ShaderDataTypeToString(uniform.Type) 
                << ", Size: " << uniform.Size << ", Offset: " << uniform.Offset 
                << ", Location: " << uniform.Location << ")\n";
        }
        
        oss << "\nResources (" << reflection.Resources.size() << "):\n";
        for (const auto& resource : reflection.Resources)
        {
            oss << "  - " << resource.Name << " (Set: " << resource.Set 
                << ", Binding: " << resource.Binding << ", Array: " << resource.ArraySize << ")\n";
        }
        
        oss << "\nVertex Inputs (" << reflection.VertexInputs.size() << "):\n";
        for (const auto& input : reflection.VertexInputs)
        {
            oss << "  - " << input.Name << " (Location: " << input.Location 
                << ", Type: " << ShaderDataTypeToString(input.Type) 
                << ", Size: " << input.Size << ")\n";
        }
        
        return oss.str();
    }

} // namespace Vortex
