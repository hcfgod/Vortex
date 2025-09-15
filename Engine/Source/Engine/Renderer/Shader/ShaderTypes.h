#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

#include <glm/glm.hpp>

namespace Vortex
{
    // ============================================================================
    // SHADER STAGE DEFINITIONS
    // ============================================================================
    
    enum class ShaderStage : uint32_t
    {
        Vertex = 0,
        TessellationControl,
        TessellationEvaluation, 
        Geometry,
        Fragment,
        Compute,
        // Ray tracing stages
        RayGeneration,
        AnyHit,
        ClosestHit,
        Miss,
        Intersection,
        Callable,
        
        Count
    };
    
    constexpr uint32_t MaxShaderStages = static_cast<uint32_t>(ShaderStage::Count);
    
    using ShaderStageFlags = uint32_t;
    constexpr ShaderStageFlags ShaderStageFlag_Vertex = 1u << static_cast<uint32_t>(ShaderStage::Vertex);
    constexpr ShaderStageFlags ShaderStageFlag_Fragment = 1u << static_cast<uint32_t>(ShaderStage::Fragment);
    constexpr ShaderStageFlags ShaderStageFlag_Compute = 1u << static_cast<uint32_t>(ShaderStage::Compute);
    constexpr ShaderStageFlags ShaderStageFlag_AllGraphics = ShaderStageFlag_Vertex | ShaderStageFlag_Fragment;
    
    // ============================================================================
    // SHADER LANGUAGES AND COMPILATION
    // ============================================================================
    
    enum class ShaderLanguage
    {
        GLSL,
        HLSL,
        SPIRV  // Pre-compiled SPIR-V binary
    };
    
    enum class ShaderOptimizationLevel
    {
        None,           // No optimization, fastest compile
        Size,           // Optimize for size
        Performance,    // Optimize for performance
        Debug           // Debug-friendly compilation
    };
    
    struct ShaderMacro
    {
        std::string Name;
        std::string Value;
        
        ShaderMacro(std::string name, std::string value = "1")
            : Name(std::move(name)), Value(std::move(value)) {}
    };
    
    using ShaderMacros = std::vector<ShaderMacro>;
    
    // ============================================================================
    // SHADER RESOURCE REFLECTION
    // ============================================================================
    
    enum class ShaderResourceType
    {
        UniformBuffer,
        StorageBuffer,
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        TextureArray,
        Sampler,
        Image,
        AtomicCounter,
        SubpassInput
    };
    
    enum class ShaderDataType
    {
        Unknown = 0,
        Bool, Int, UInt, Float, Double,
        Vec2, Vec3, Vec4,
        IVec2, IVec3, IVec4,
        UVec2, UVec3, UVec4,
        Mat2, Mat3, Mat4,
        Struct
    };
    
    struct ShaderUniform
    {
        std::string Name;
        ShaderDataType Type;
        uint32_t Size;           // Size in bytes
        uint32_t Offset;         // Offset within uniform buffer
        uint32_t ArraySize;      // 0 if not an array, else array size
        uint32_t Location;       // Binding location
    };
    
    struct ShaderResource
    {
        std::string Name;
        ShaderResourceType Type;
        uint32_t Set;            // Descriptor set
        uint32_t Binding;        // Binding within set
        uint32_t ArraySize;      // Array size (1 if not array)
        ShaderStageFlags Stages; // Stages this resource is used in
    };
    
    struct ShaderVertexInput
    {
        std::string Name;
        uint32_t Location;
        ShaderDataType Type;
        uint32_t Size;
    };
    
    struct ShaderReflectionData
    {
        std::vector<ShaderUniform> Uniforms;
        std::vector<ShaderResource> Resources;
        std::vector<ShaderVertexInput> VertexInputs;
        std::unordered_map<uint32_t, std::vector<ShaderUniform>> UniformBuffers;
        
        // Compute shader specific
        glm::uvec3 LocalSize{0, 0, 0};  // Local workgroup size
        
        // Statistics
        uint32_t InstructionCount = 0;
        uint32_t MemoryUsage = 0;
    };
    
    // ============================================================================
    // COMPILATION RESULTS
    // ============================================================================
    
    enum class ShaderCompileStatus
    {
        Success,
        CompilationError,
        LinkingError,
        ReflectionError,
        CacheError,
        FileNotFound,
        InvalidInput
    };
    
    struct ShaderCompileError
    {
        std::string Message;
        std::string File;
        uint32_t Line = 0;
        uint32_t Column = 0;
        ShaderStage Stage = ShaderStage::Vertex;
    };
    
    struct CompiledShader
    {
        ShaderCompileStatus Status = ShaderCompileStatus::CompilationError;
        std::vector<uint32_t> SpirV;
        std::vector<ShaderCompileError> Errors;
        ShaderReflectionData Reflection;
        
        // Metadata
        ShaderStage Stage = ShaderStage::Vertex;
        ShaderLanguage SourceLanguage = ShaderLanguage::GLSL;
        std::string SourceFile;
        uint64_t SourceHash = 0;
        
        bool IsValid() const { return Status == ShaderCompileStatus::Success && !SpirV.empty(); }
    };
    
    struct ShaderProgram
    {
        std::unordered_map<ShaderStage, CompiledShader> Shaders;
        ShaderReflectionData CombinedReflection;
        uint32_t ProgramId = 0;  // Graphics API handle
        
        bool IsComplete() const 
        {
            // Must have at least vertex and fragment for graphics pipeline
            return Shaders.find(ShaderStage::Vertex) != Shaders.end() &&
                   Shaders.find(ShaderStage::Fragment) != Shaders.end();
        }
        
        bool IsComputeProgram() const
        {
            return Shaders.size() == 1 && 
                   Shaders.find(ShaderStage::Compute) != Shaders.end();
        }
    };
    
    // ============================================================================
    // SHADER COMPILATION OPTIONS
    // ============================================================================
    
    struct ShaderCompileOptions
    {
        ShaderOptimizationLevel OptimizationLevel = ShaderOptimizationLevel::None;
        std::unordered_map<std::string, std::string> Macros;
        std::vector<std::string> IncludePaths;
        
        bool GenerateDebugInfo = false;
        bool EnableWarnings = true;
        bool TreatWarningsAsErrors = false;
        bool EnableHotReload = false;
        
        // Target environment
        std::string TargetProfile = "vulkan1.1";  // vulkan1.0, vulkan1.1, opengl4.5, etc.
        
        // Validation
        bool EnableValidation = true;
        bool ValidateAfterOptimization = false;
        
        // Caching
        bool EnableCaching = true;
        std::string CacheDirectory = "cache/shaders/";

        // Optional hardware capability hints to drive permutations or limits
        struct Caps
        {
            uint32_t MaxTextureUnits = 0;
            uint32_t MaxSamples = 0;
            bool SupportsGeometry = false;
            bool SupportsCompute = false;
            bool SupportsMultiDrawIndirect = false;
            bool SRGBFramebufferCapable = false;
        } HardwareCaps;
    };
    
    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================
    
    const char* ShaderStageToString(ShaderStage stage);
    const char* ShaderLanguageToString(ShaderLanguage language);
    const char* ShaderDataTypeToString(ShaderDataType type);
    uint32_t GetShaderDataTypeSize(ShaderDataType type);
    
    std::string ShaderStageToFileExtension(ShaderStage stage);
    ShaderStage FileExtensionToShaderStage(const std::string& extension);
    
} // namespace Vortex
