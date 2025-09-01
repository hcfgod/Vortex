#include "vxpch.h"
#include "SDLWindow.h"
#include "SDL3Manager.h"
#include "Core/Debug/Log.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "SDL3/SDL.h"

namespace Vortex
{
    SDLWindow::SDLWindow(const WindowProperties& props)
        : m_Properties(props)
    {
        if (!CreateSDLWindow())
        {
            VX_CORE_ERROR("Failed to create SDL window");
        }
    }

    SDLWindow::~SDLWindow()
    {
        DestroyWindow();
    }

    bool SDLWindow::CreateSDLWindow()
    {
        if (!SDL3Manager::IsInitialized())
        {
            VX_CORE_ERROR("Cannot create window: SDL3Manager not initialized");
            return false;
        }

        // Build window flags
        SDL_WindowFlags flags = 0;
        
        if (m_Properties.Resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        
        if (m_Properties.Fullscreen)
            flags |= SDL_WINDOW_FULLSCREEN;
            
        if (m_Properties.HighDPI)
            flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

        // If using OpenGL as the GraphicsAPI, mark this window as OpenGL-capable
        if (GetGraphicsAPI() == GraphicsAPI::OpenGL)
            flags |= SDL_WINDOW_OPENGL;

        // Create the window
        m_Window = SDL_CreateWindow(
            m_Properties.Title.c_str(),
            m_Properties.Width,
            m_Properties.Height,
            flags
        );

        if (!m_Window)
        {
            VX_CORE_ERROR("Failed to create SDL window: {0}", SDL_GetError());
            return false;
        }

        m_WindowID = SDL_GetWindowID(m_Window);
        
        VX_CORE_INFO("Window created successfully: '{0}' ({1}x{2})", 
                     m_Properties.Title, m_Properties.Width, m_Properties.Height);

        return true;
    }

    void SDLWindow::DestroyWindow()
    {
        if (m_Window)
        {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
            m_WindowID = 0;
            VX_CORE_INFO("Window destroyed");
        }
    }

    uint32_t SDLWindow::GetWindowID() const
    {
        return static_cast<uint32_t>(m_WindowID);
    }

    void SDLWindow::SetTitle(const std::string& title)
    {
        if (!m_Window) return;
        
        SDL_SetWindowTitle(m_Window, title.c_str());
        m_Properties.Title = title;
    }

    std::string SDLWindow::GetTitle() const
    {
        if (!m_Window) return m_Properties.Title;
        
        const char* title = SDL_GetWindowTitle(m_Window);
        return title ? std::string(title) : std::string();
    }

    void SDLWindow::SetSize(int width, int height)
    {
        if (!m_Window) return;
        
        SDL_SetWindowSize(m_Window, width, height);
        m_Properties.Width = width;
        m_Properties.Height = height;
    }

    void SDLWindow::GetSize(int& width, int& height) const
    {
        if (!m_Window)
        {
            width = m_Properties.Width;
            height = m_Properties.Height;
            return;
        }
        
        SDL_GetWindowSize(m_Window, &width, &height);
    }

    int SDLWindow::GetWidth() const
    {
        int width, height;
        GetSize(width, height);
        return width;
    }

    int SDLWindow::GetHeight() const
    {
        int width, height;
        GetSize(width, height);
        return height;
    }

    void SDLWindow::SetPosition(int x, int y)
    {
        if (!m_Window) return;
        
        SDL_SetWindowPosition(m_Window, x, y);
    }

    void SDLWindow::GetPosition(int& x, int& y) const
    {
        if (!m_Window)
        {
            x = y = 0;
            return;
        }
        
        SDL_GetWindowPosition(m_Window, &x, &y);
    }

    void SDLWindow::SetFullscreen(bool fullscreen)
    {
        if (!m_Window) return;
        
        SDL_SetWindowFullscreen(m_Window, fullscreen);
        m_Properties.Fullscreen = fullscreen;
    }

    bool SDLWindow::IsFullscreen() const
    {
        if (!m_Window) return m_Properties.Fullscreen;
        
        SDL_WindowFlags flags = SDL_GetWindowFlags(m_Window);
        return (flags & SDL_WINDOW_FULLSCREEN) != 0;
    }

    void SDLWindow::Show()
    {
        if (!m_Window) return;
        SDL_ShowWindow(m_Window);
    }

    void SDLWindow::Hide()
    {
        if (!m_Window) return;
        SDL_HideWindow(m_Window);
    }

    bool SDLWindow::IsVisible() const
    {
        if (!m_Window) return false;
        
        SDL_WindowFlags flags = SDL_GetWindowFlags(m_Window);
        return (flags & SDL_WINDOW_HIDDEN) == 0;
    }

    void SDLWindow::Minimize()
    {
        if (!m_Window) return;
        SDL_MinimizeWindow(m_Window);
    }

    void SDLWindow::Maximize()
    {
        if (!m_Window) return;
        SDL_MaximizeWindow(m_Window);
    }

    void SDLWindow::Restore()
    {
        if (!m_Window) return;
        SDL_RestoreWindow(m_Window);
    }

    void SDLWindow::Center()
    {
        if (!m_Window) return;
        
        SetPosition(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    // VSync control has been moved to RenderSystem/GraphicsContext
    // This ensures proper synchronization with the graphics API

    void SDLWindow::UpdateProperties()
    {
        if (!m_Window) return;
        
        // Update properties from current window state
        GetSize(m_Properties.Width, m_Properties.Height);
        m_Properties.Fullscreen = IsFullscreen();
        m_Properties.Title = GetTitle();
    }

    bool SDLWindow::ProcessEvent(const SDL_Event& event)
    {
        if (!m_Window) return false;
        
        // Only handle events for our window
        if (event.window.windowID != m_WindowID)
            return false;

        switch (event.type)
        {
            case SDL_EVENT_WINDOW_RESIZED:
            {
                m_Properties.Width = event.window.data1;
                m_Properties.Height = event.window.data2;
                VX_CORE_INFO("Window resized: {0}x{1}", m_Properties.Width, m_Properties.Height);
                return true;
            }
            
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                VX_CORE_INFO("Window close requested");
                return true;
            }
            
            case SDL_EVENT_WINDOW_MINIMIZED:
            {
                VX_CORE_INFO("Window minimized");
                return true;
            }
            
            case SDL_EVENT_WINDOW_MAXIMIZED:
            {
                VX_CORE_INFO("Window maximized");
                return true;
            }
            
            case SDL_EVENT_WINDOW_RESTORED:
            {
                VX_CORE_INFO("Window restored");
                return true;
            }
            
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                VX_CORE_TRACE("Window gained focus");
                return true;
            }
            
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                VX_CORE_TRACE("Window lost focus");
                return true;
            }
        }
        
        return false;
    }
}
