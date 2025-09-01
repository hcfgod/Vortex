#pragma once

#include "Core/Window.h"
#include "SDL3/SDL.h"

namespace Vortex
{
    /**
     * @brief SDL3-specific window implementation
     * 
     * This class encapsulates SDL window creation, management, and event handling.
     * It provides the concrete implementation of the abstract Window interface.
     */
    class SDLWindow : public Window
    {
    public:
        /**
         * @brief Create an SDL window with specified properties
         * @param props Window properties (title, size, etc.)
         */
        explicit SDLWindow(const WindowProperties& props = WindowProperties{});
        
        /**
         * @brief Destructor - automatically destroys SDL window
         */
        ~SDLWindow() override;

        // Implement Window interface
        bool IsValid() const override { return m_Window != nullptr; }
        uint32_t GetWindowID() const override;
        void SetTitle(const std::string& title) override;
        std::string GetTitle() const override;
        void SetSize(int width, int height) override;
        void GetSize(int& width, int& height) const override;
        int GetWidth() const override;
        int GetHeight() const override;
        void SetPosition(int x, int y) override;
        void GetPosition(int& x, int& y) const override;
        void SetFullscreen(bool fullscreen) override;
        bool IsFullscreen() const override;
        void Show() override;
        void Hide() override;
        bool IsVisible() const override;
        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void Center() override;
        const WindowProperties& GetProperties() const override { return m_Properties; }
        bool ProcessEvent(const SDL_Event& event) override;
        void* GetNativeHandle() const override { return m_Window; }

        /**
         * @brief Get the native SDL window handle (typed)
         * @return SDL_Window pointer or nullptr if invalid
         */
        SDL_Window* GetSDLWindow() const { return m_Window; }

    private:
        /**
         * @brief Create the SDL window with current properties
         * @return True if successful, false otherwise
         */
        bool CreateSDLWindow();

        /**
         * @brief Destroy the SDL window and cleanup
         */
        void DestroyWindow();

        /**
         * @brief Update internal properties from current window state
         */
        void UpdateProperties();

        SDL_Window* m_Window = nullptr;
        WindowProperties m_Properties;
        SDL_WindowID m_WindowID = 0;
    };
}
