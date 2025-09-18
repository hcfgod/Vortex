#include "vxpch.h"
#include "Camera.h"
#include "Core/Debug/Log.h"
#include "Engine/Time/Time.h"

namespace Vortex
{
    CameraSystem::CameraSystem() : EngineSystem("CameraSystem", SystemPriority::High)
    {
        VX_CORE_INFO("CameraSystem created");
    }

    CameraSystem::~CameraSystem()
    {
        // Ensure shutdown if not already done
        if (IsInitialized())
        {
            Shutdown();
        }

        VX_CORE_INFO("CameraSystem destroyed");
    }

    Result<void> CameraSystem::Initialize()
    {
        VX_CORE_INFO("CameraSystem initializing...");
        
        // Clear any existing cameras
        m_Cameras.clear();
        m_ActiveCamera.reset();
        
        MarkInitialized();
        VX_CORE_INFO("CameraSystem initialized successfully");
        
        return Result<void>();
    }

    Result<void> CameraSystem::Update()
    {
        // Update all registered cameras
        for (auto& camera : m_Cameras)
        {
            if (camera)
            {
                camera->OnUpdate(Time::GetDeltaTime());
            }
        }
        
        return Result<void>();
    }

    Result<void> CameraSystem::Shutdown()
    {
        VX_CORE_INFO("CameraSystem shutting down...");
        
        // Clear all cameras
        m_Cameras.clear();
        m_ActiveCamera.reset();
        
        MarkShutdown();
        VX_CORE_INFO("CameraSystem shutdown complete");
        
        return Result<void>();
    }

    void CameraSystem::Register(const std::shared_ptr<Camera>& camera)
    {
        if (!camera) 
        {
            VX_CORE_WARN("Attempted to register null camera");
            return;
        }
        
        m_Cameras.emplace_back(camera);
        
        // Set as active camera if none is currently active
        if (!m_ActiveCamera)
        {
            m_ActiveCamera = camera;
            VX_CORE_INFO("Set first registered camera as active camera");
        }
        
        VX_CORE_INFO("Camera registered successfully. Total cameras: {}", m_Cameras.size());
    }

    void CameraSystem::Unregister(const std::shared_ptr<Camera>& camera)
    {
        if (!camera)
        {
            VX_CORE_WARN("Attempted to unregister null camera");
            return;
        }
        
        auto it = std::find(m_Cameras.begin(), m_Cameras.end(), camera);
        if (it != m_Cameras.end())
        {
            m_Cameras.erase(it);
            
            // If this was the active camera, clear it
            if (m_ActiveCamera == camera)
            {
                m_ActiveCamera.reset();
                VX_CORE_INFO("Active camera unregistered");
            }
            
            VX_CORE_INFO("Camera unregistered successfully. Total cameras: {}", m_Cameras.size());
        }
        else
        {
            VX_CORE_WARN("Attempted to unregister camera that was not registered");
        }
    }

    void CameraSystem::OnWindowResize(uint32_t width, uint32_t height)
    {
        for (auto& camera : m_Cameras)
        {
            if (camera)
            {
                camera->SetViewportSize(width, height);
            }
        }
        
        VX_CORE_INFO("Updated all cameras for window resize: {}x{}", width, height);
    }
}
