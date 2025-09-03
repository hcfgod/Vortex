#include "vxpch.h"
#include "ShaderReflection.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/ErrorCodes.h"

#ifdef VX_SPIRV_CROSS_AVAILABLE
    #include <spirv_cross/spirv_cross.hpp>
    #include <spirv_cross/spirv_glsl.hpp>
#endif

namespace Vortex::Shader
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

    bool ShaderReflection::ValidateShaderCompatibility(const ShaderReflectionData& vertexReflection,
                                                       const ShaderReflectionData& fragmentReflection)
    {
        // Check that vertex outputs match fragment inputs
        // For now, we'll do a basic validation - can be enhanced later
        
        // Ensure critical uniforms are present in both if needed
        std::unordered_set<std::string> vertexUniforms, fragmentUniforms;
        
        for (const auto& uniform : vertexReflection.Uniforms)
        {
            vertexUniforms.insert(uniform.Name);
        }
        
        for (const auto& uniform : fragmentReflection.Uniforms)
        {
            fragmentUniforms.insert(uniform.Name);
        }
        
        // Basic validation - both shaders should be able to link
        return true;  // For now, assume compatibility
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

} // namespace Vortex::Shader
