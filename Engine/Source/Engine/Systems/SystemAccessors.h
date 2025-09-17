#pragma once

#include "Core/Application.h"
#include "Engine/Engine.h"

namespace Vortex
{
    /// <summary>
    /// Global system accessors for internal engine use.
    /// These functions allow systems to access other systems without direct dependencies.
    /// </summary>
    
    inline Application* GetApp() { return Application::Get(); }

    inline Engine* GetEngine()
    {
        auto* a = Application::Get();
        return a ? a->GetEngine() : nullptr;
    }

    template<typename T>
    inline T* Sys()
    {
        auto* e = GetEngine();
        return e ? e->GetSystem<T>() : nullptr;
    }

    template<typename T>
    inline std::shared_ptr<T> SysShared()
    {
        auto* e = GetEngine();
        return e ? e->GetSystemShared<T>() : std::shared_ptr<T>();
    }
}
