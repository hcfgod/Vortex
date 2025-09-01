#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include "Core/Debug/ErrorCodes.h"
#include <memory>
#include <SDL3/SDL.h>

// Forward declare SDL structures to avoid including SDL headers here
struct SDL_Window;

namespace Vortex
{
    /**
     * @brief OpenGL implementation of GraphicsContext using SDL3 and GLAD
     */
    class SDL_OpenGLGraphicsContext : public GraphicsContext
    {
    public:
        /**
         * @brief Construct OpenGL context for the given SDL window
         * @param window SDL window handle
         */
        explicit SDL_OpenGLGraphicsContext(SDL_Window* window);

        /**
         * @brief Destructor - cleans up OpenGL context
         */
        ~SDL_OpenGLGraphicsContext() override;

        // Disable copy constructor and assignment
        SDL_OpenGLGraphicsContext(const SDL_OpenGLGraphicsContext&) = delete;
        SDL_OpenGLGraphicsContext& operator=(const SDL_OpenGLGraphicsContext&) = delete;

        // ============================================================================
        // GraphicsContext Interface Implementation
        // ============================================================================

        /**
         * @brief Initialize the OpenGL context
         * @param props Context initialization properties
         * @return Success/failure result
         */
        Result<void> Initialize(const GraphicsContextProps& props) override;

        /**
         * @brief Shutdown the context
         * @return Success/failure result
         */
        Result<void> Shutdown() override;

        /**
         * @brief Make this context current
         * @return Success/failure result
         */
        Result<void> MakeCurrent() override;

        /**
         * @brief Present the current frame (swap buffers)
         * @return Success/failure result
         */
        Result<void> Present() override;

        /**
         * @brief Resize the context
         * @param width New width
         * @param height New height
         * @return Success/failure result
         */
        Result<void> Resize(uint32_t width, uint32_t height) override;

        /**
         * @brief Set VSync mode (legacy method)
         * @param enabled True to enable VSync, false to disable
         * @return Success/failure result
         */
        Result<void> SetVSync(bool enabled) override;

        /**
         * @brief Set the VSync mode with fine-grained control
         * @param mode The VSync mode to set
         * @return Success/failure result
         */
        Result<void> SetVSyncMode(VSyncMode mode) override;

        /**
         * @brief Check if VSync is enabled
         * @return True if VSync is enabled
         */
        bool IsVSyncEnabled() const override { return m_CurrentVSyncMode != VSyncMode::Disabled; }

        /**
         * @brief Get the current VSync mode
         * @return Current VSync mode
         */
        VSyncMode GetVSyncMode() const override { return m_CurrentVSyncMode; }

        /**
         * @brief Get supported VSync modes for this OpenGL context
         * @return Vector of supported VSync modes
         */
        std::vector<VSyncMode> GetSupportedVSyncModes() const override;

        /**
         * @brief Check if the context is valid
         * @return True if context is valid and ready to use
         */
        bool IsValid() const override;


        /**
         * @brief Get information about the graphics context capabilities
         * @return Context information structure
         */
        const GraphicsContextInfo& GetInfo() const override;

        /**
         * @brief Get the current framebuffer size
         * @param width Output parameter for width
         * @param height Output parameter for height
         */
        void GetFramebufferSize(uint32_t& width, uint32_t& height) const override;

        // ============================================================================
        // OpenGL-Specific Methods
        // ============================================================================

        /**
         * @brief Get the SDL window handle
         * @return SDL window pointer
         */
        SDL_Window* GetWindow() const { return m_Window; }

        /**
         * @brief Get the OpenGL context handle
         * @return SDL OpenGL context handle
         */
        SDL_GLContext GetGLContext() const { return m_Context; }

        /**
         * @brief Get OpenGL version string
         * @return OpenGL version string
         */
        std::string GetOpenGLVersion() const;

        /**
         * @brief Get OpenGL renderer string
         * @return OpenGL renderer string
         */
        std::string GetOpenGLRenderer() const;

        /**
         * @brief Get OpenGL vendor string
         * @return OpenGL vendor string
         */
        std::string GetOpenGLVendor() const;

        /**
         * @brief Check if a specific OpenGL extension is supported
         * @param extension Extension name
         * @return True if extension is supported
         */
        bool IsExtensionSupported(const std::string& extension) const;

    private:
        SDL_Window* m_Window = nullptr;            ///< SDL window handle
        SDL_GLContext m_Context = nullptr;         ///< OpenGL context handle
        bool m_Initialized = false;               ///< Initialization state
        VSyncMode m_CurrentVSyncMode = VSyncMode::Disabled;  ///< Current VSync mode
        mutable std::vector<VSyncMode> m_SupportedVSyncModes; ///< Cached supported VSync modes
        mutable bool m_VSyncModesCached = false;  ///< Whether supported modes have been queried
        
        // Legacy compatibility
        bool m_VSyncEnabled = false;              ///< Legacy VSync state (deprecated)
        
        // Cached framebuffer size
        mutable uint32_t m_FramebufferWidth = 0;
        mutable uint32_t m_FramebufferHeight = 0;
        mutable bool m_FramebufferSizeCached = false;

        /**
         * @brief Query and cache framebuffer size
         */
        void CacheFramebufferSize() const;

        /**
         * @brief Setup OpenGL debug callback (if available)
         */
        void SetupDebugCallback();

        /**
         * @brief Query and cache supported VSync modes
         */
        void CacheSupportedVSyncModes() const;

        /**
         * @brief Convert VSyncMode to SDL swap interval
         * @param mode VSync mode
         * @return SDL swap interval value
         */
        int VSyncModeToSDLInterval(VSyncMode mode) const;

        /**
         * @brief Test if a specific VSync mode is supported
         * @param mode VSync mode to test
         * @return True if supported
         */
        bool TestVSyncModeSupport(VSyncMode mode) const;
    };
}
