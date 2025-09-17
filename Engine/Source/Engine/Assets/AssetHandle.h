#pragma once

#include "Core/Common/UUID.h"
#include <optional>
#include <functional>

namespace Vortex
{
    class Asset;
    class AssetSystem;

    template<typename T>
    class AssetHandle
    {
    public:
        AssetHandle() = default;
        AssetHandle(AssetSystem* system, UUID id)
            : m_System(system), m_Id(id)
        {
            if (IsValid()) m_System->Acquire(m_Id);
        }

        // Copy constructor
        AssetHandle(const AssetHandle& other)
            : m_System(other.m_System), m_Id(other.m_Id)
        {
            if (IsValid()) m_System->Acquire(m_Id);
        }

        // Copy assignment
        AssetHandle& operator=(const AssetHandle& other)
        {
            if (this == &other) return *this;
            // Release current
            if (IsValid()) m_System->Release(m_Id);
            m_System = other.m_System;
            m_Id = other.m_Id;
            if (IsValid()) m_System->Acquire(m_Id);
            return *this;
        }

        // Move constructor
        AssetHandle(AssetHandle&& other) noexcept
            : m_System(other.m_System), m_Id(other.m_Id)
        {
            other.m_System = nullptr;
            other.m_Id = UUID::Invalid;
        }

        // Move assignment
        AssetHandle& operator=(AssetHandle&& other) noexcept
        {
            if (this == &other) return *this;
            if (IsValid()) m_System->Release(m_Id);
            m_System = other.m_System;
            m_Id = other.m_Id;
            other.m_System = nullptr;
            other.m_Id = UUID::Invalid;
            return *this;
        }

        ~AssetHandle()
        {
            if (IsValid()) m_System->Release(m_Id);
        }

        bool IsValid() const { return m_System != nullptr && m_Id != UUID::Invalid && m_System->Exists(m_Id); }
        const UUID& GetId() const { return m_Id; }

        // Query state/progress via system
        bool IsLoaded() const;
        float GetProgress() const;

        // Try get the underlying asset pointer (nullptr if not loaded or wrong type)
        const T* TryGet() const;
        T* TryGet();

        // Convenience
        explicit operator bool() const { return IsValid(); }

    private:
        AssetSystem* m_System = nullptr; // non-owning
        UUID m_Id = UUID::Invalid;
    };
}

