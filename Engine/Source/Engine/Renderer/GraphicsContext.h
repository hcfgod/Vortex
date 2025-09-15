#pragma once

#include "Core/Debug/ErrorCodes.h"

namespace Vortex
{
    /**
     * @brief Graphics context creation parameters for different APIs
     */
    enum class GraphicsAPI : uint8_t
    {
        None = 0,
        OpenGL,
        DirectX11,
        DirectX12,
        Vulkan,
        Metal
    };

    /**
     * @brief VSync modes for controlling frame presentation timing
     */
    enum class VSyncMode : uint8_t
    {
        Disabled = 0,    ///< VSync disabled - no synchronization
        Enabled,         ///< Standard VSync - sync to display refresh rate
        Adaptive,        ///< Adaptive VSync - sync when framerate > refresh rate, otherwise disabled
        Fast,            ///< Fast VSync - similar to adaptive but with different fallback behavior
        Mailbox          ///< Mailbox mode - triple buffering (Vulkan-specific, maps to Enabled for others)
    };

    /**
     * @brief Graphics context properties for initialization
     */
    struct GraphicsContextProps
    {
        void* WindowHandle = nullptr;  // Platform-specific window handle
        uint32_t WindowWidth = 1280;
        uint32_t WindowHeight = 720;
        bool DebugMode = true;        // Enable debug layer/validation in Debug builds
        uint32_t MSAASamples = 1;     // Anti-aliasing samples (1 = disabled)
        std::string ApplicationName = "Vortex Engine";
        
        // Note: VSync is now controlled via RenderSystem::SetRenderSettings()
        // This ensures single source of truth and proper synchronization
    };

    /**
     * @brief Graphics context capabilities and limits
     */
    struct GraphicsContextInfo
    {
        std::string RendererName;
        std::string RendererVersion;
        std::string ShadingLanguageVersion;

        // Limits/caps
        uint32_t MaxTextureSize = 0;
        uint32_t MaxVertexAttributes = 0;
        uint32_t MaxUniformBufferBindings = 0;
        uint32_t MaxColorAttachments = 0;
        uint32_t MaxCombinedTextureUnits = 0;
        uint32_t MaxSamples = 0;
        uint32_t MaxUniformBlockSize = 0;
        uint32_t MaxSSBOBindings = 0;

        // Default framebuffer info
        uint32_t DepthBits = 0;
        uint32_t StencilBits = 0;
        uint32_t SampleCount = 0; // MSAA samples for default framebuffer
        bool SRGBFramebufferCapable = false;

        // Feature support flags
        bool SupportsGeometryShaders = false;
        bool SupportsComputeShaders = false;
        bool SupportsMultiDrawIndirect = false;

        // Extensions list (if available)
        std::vector<std::string> Extensions;
    };

    /**
     * @brief Abstract graphics context interface
     * 
     * This class provides a unified interface for different graphics APIs (OpenGL, DirectX, Vulkan, etc.)
     * Each implementation handles API-specific context creation, management, and resource binding.
     * 
     * @note All methods are designed to be called from the main rendering thread unless specified otherwise
     */
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        /**
         * @brief Initialize the graphics context
         * @param props Context initialization properties
         * @return Success/failure result
         */
        virtual Result<void> Initialize(const GraphicsContextProps& props) = 0;

        /**
         * @brief Shutdown the graphics context and clean up resources
         * @return Success/failure result
         */
        virtual Result<void> Shutdown() = 0;

        /**
         * @brief Make this context current for rendering operations
         * @return Success/failure result
         */
        virtual Result<void> MakeCurrent() = 0;

        /**
         * @brief Present the back buffer to the screen
         * @return Success/failure result
         */
        virtual Result<void> Present() = 0;

        /**
         * @brief Resize the context's framebuffer
         * @param width New framebuffer width
         * @param height New framebuffer height
         * @return Success/failure result
         */
        virtual Result<void> Resize(uint32_t width, uint32_t height) = 0;

        /**
         * @brief Enable or disable vertical synchronization (legacy method)
         * @param enabled True to enable VSync, false to disable
         * @return Success/failure result
         * @note This method maps to SetVSyncMode(enabled ? VSyncMode::Enabled : VSyncMode::Disabled)
         */
        virtual Result<void> SetVSync(bool enabled) = 0;

        /**
         * @brief Set the VSync mode with fine-grained control
         * @param mode The VSync mode to set
         * @return Success/failure result
         */
        virtual Result<void> SetVSyncMode(VSyncMode mode) = 0;

        /**
         * @brief Check if VSync is currently enabled
         * @return True if VSync is enabled, false otherwise
         */
        virtual bool IsVSyncEnabled() const = 0;

        /**
         * @brief Get the current VSync mode
         * @return Current VSync mode
         */
        virtual VSyncMode GetVSyncMode() const = 0;

        /**
         * @brief Get supported VSync modes for this context
         * @return Vector of supported VSync modes
         */
        virtual std::vector<VSyncMode> GetSupportedVSyncModes() const = 0;

        /**
         * @brief Get information about the graphics context capabilities
         * @return Context information structure
         */
        virtual const GraphicsContextInfo& GetInfo() const = 0;

        /**
         * @brief Get the current framebuffer size
         * @param width Output parameter for width
         * @param height Output parameter for height
         */
        virtual void GetFramebufferSize(uint32_t& width, uint32_t& height) const = 0;

        /**
         * @brief Check if the context is currently valid and initialized
         * @return True if context is valid, false otherwise
         */
        virtual bool IsValid() const = 0;

        // Note: Per-context API querying is redundant with the global GetGraphicsAPI().
        // If needed in the future for multi-API scenarios, this can be reintroduced.

        /**
         * @brief Get debug information about the current state (Debug builds only)
         * @return Debug info string
         */
        virtual std::string GetDebugInfo() const { return "Debug info not available"; }

        // Factory method for creating platform-specific contexts
        /**
         * @brief Create a graphics context for the current platform/API
         * @return Unique pointer to the created context, nullptr on failure
         */
        static std::unique_ptr<GraphicsContext> Create();

    protected:
        GraphicsContextProps m_Props;
        GraphicsContextInfo m_Info;
    };


    /**
     * @brief Get the currently selected graphics API
     * @return The active graphics API
     */
    GraphicsAPI GetGraphicsAPI();

    /**
     * @brief Set the graphics API to use (must be called before context creation)
     * @param api The graphics API to use
     */
    void SetGraphicsAPI(GraphicsAPI api);

    /**
     * @brief Convert GraphicsAPI enum to string for debugging
     * @param api The graphics API enum value
     * @return String representation of the API
     */
    const char* GraphicsAPIToString(GraphicsAPI api);

    /**
     * @brief Convert VSyncMode enum to string for debugging
     * @param mode The VSync mode enum value
     * @return String representation of the VSync mode
     */
    const char* VSyncModeToString(VSyncMode mode);

    /**
     * @brief Convert string to VSyncMode enum
     * @param modeStr String representation of VSync mode
     * @return VSyncMode enum value, or VSyncMode::Disabled if invalid
     */
    VSyncMode StringToVSyncMode(const std::string& modeStr);
}
