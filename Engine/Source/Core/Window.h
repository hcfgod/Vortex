#pragma once

#include <memory>
#include <string>

union SDL_Event;

namespace Vortex
{
    /**
     * @brief Window properties structure
     */
    struct WindowProperties
    {
        std::string Title = "Vortex Engine";
        int Width = 1280;
        int Height = 720;
        bool Fullscreen = false;
        bool Resizable = true;
        bool HighDPI = true;
    };

    /**
     * @brief Abstract base class for platform-specific window implementations
     * 
     * This provides a common interface for all window implementations (SDL, GLFW, etc.)
     * Platform-specific details are handled by derived classes.
     */
    class Window
    {
    public:
        virtual ~Window() = default;

        // Non-copyable but movable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = default;
        Window& operator=(Window&&) = default;

        /**
         * @brief Check if window is valid and open
         * @return True if window is valid, false otherwise
         */
        virtual bool IsValid() const = 0;

        /**
         * @brief Get window ID (platform-specific)
         * @return Platform-specific window ID
         */
        virtual uint32_t GetWindowID() const = 0;

        /**
         * @brief Set window title
         * @param title New window title
         */
        virtual void SetTitle(const std::string& title) = 0;

        /**
         * @brief Get current window title
         * @return Window title string
         */
        virtual std::string GetTitle() const = 0;

        /**
         * @brief Set window size
         * @param width New width in pixels
         * @param height New height in pixels
         */
        virtual void SetSize(int width, int height) = 0;

        /**
         * @brief Get current window size
         * @param width Output parameter for width
         * @param height Output parameter for height
         */
        virtual void GetSize(int& width, int& height) const = 0;

        /**
         * @brief Get current window width
         * @return Window width in pixels
         */
        virtual int GetWidth() const = 0;

        /**
         * @brief Get current window height
         * @return Window height in pixels
         */
        virtual int GetHeight() const = 0;

        /**
         * @brief Set window position
         * @param x X coordinate
         * @param y Y coordinate
         */
        virtual void SetPosition(int x, int y) = 0;

        /**
         * @brief Get current window position
         * @param x Output parameter for X coordinate
         * @param y Output parameter for Y coordinate
         */
        virtual void GetPosition(int& x, int& y) const = 0;

        /**
         * @brief Set fullscreen mode
         * @param fullscreen True for fullscreen, false for windowed
         */
        virtual void SetFullscreen(bool fullscreen) = 0;

        /**
         * @brief Check if window is fullscreen
         * @return True if fullscreen, false if windowed
         */
        virtual bool IsFullscreen() const = 0;

        /**
         * @brief Show the window
         */
        virtual void Show() = 0;

        /**
         * @brief Hide the window
         */
        virtual void Hide() = 0;

        /**
         * @brief Check if window is visible
         * @return True if visible, false if hidden
         */
        virtual bool IsVisible() const = 0;

        /**
         * @brief Minimize the window
         */
        virtual void Minimize() = 0;

        /**
         * @brief Maximize the window
         */
        virtual void Maximize() = 0;

        /**
         * @brief Restore the window from minimized/maximized state
         */
        virtual void Restore() = 0;

        /**
         * @brief Center the window on the screen
         */
        virtual void Center() = 0;

        /**
         * @brief Get the current window properties
         * @return Reference to window properties
         */
        virtual const WindowProperties& GetProperties() const = 0;

        /**
         * @brief Process platform-specific window events
         * @param event Platform-specific event (SDL_Event, etc.)
         * @return True if event was handled, false otherwise
         */
        virtual bool ProcessEvent(const SDL_Event& event) = 0;

        /**
         * @brief Get native window handle (platform-specific)
         * @return Platform-specific window handle (SDL_Window*, HWND, etc.)
         * 
         * WARNING: This breaks abstraction! Use only when interfacing with
         * platform-specific APIs (like graphics contexts, input systems, etc.)
         */
        virtual void* GetNativeHandle() const = 0;

    protected:
        // Protected constructor - only derived classes can instantiate
        Window() = default;
    };

    /**
     * @brief Factory function to create platform-specific window instances
     * @param props Window properties
     * @return Unique pointer to platform-specific window implementation
     */
    std::unique_ptr<Window> CreatePlatformWindow(const WindowProperties& props = WindowProperties{});
}
