#pragma once

#include "Core/Common/UUID.h"
#include <string>
#include <vector>
#include <atomic>
#include <memory>

namespace Vortex
{
    enum class AssetType : uint32_t
    {
        Unknown = 0,
        Shader  = 1,
        Texture = 2,
        // Mesh, Material, Audio, ... (future)
    };

    enum class AssetState : uint32_t
    {
        Unloaded = 0,
        Loading,
        Loaded,
        Failed,
        Unloading
    };

    class Asset
    {
    public:
        explicit Asset(AssetType type, const std::string& name)
            : m_Id(UUID::Generate()), m_Type(type), m_Name(name) {}
        virtual ~Asset() = default;

        const UUID& GetId() const { return m_Id; }
        const std::string& GetName() const { return m_Name; }
        AssetType GetType() const { return m_Type; }

        AssetState GetState() const { return m_State; }
        void SetState(AssetState s) { m_State = s; }

        // Loading progress 0..1
        float GetProgress() const { return m_Progress.load(); }
        void  SetProgress(float p) { m_Progress.store(p); }

        // Dependencies by UUID (optional)
        const std::vector<UUID>& GetDependencies() const { return m_Dependencies; }
        void AddDependency(const UUID& id) { m_Dependencies.push_back(id); }

        // Ref counting managed by AssetSystem via AssetHandle
        uint32_t AddRef() { return ++m_RefCount; }
        uint32_t ReleaseRef() { return --m_RefCount; }
        uint32_t GetRefCount() const { return m_RefCount.load(); }

    private:
        UUID m_Id;
        AssetType m_Type = AssetType::Unknown;
        std::string m_Name;

        std::atomic<uint32_t> m_RefCount{0};
        std::atomic<float> m_Progress{0.0f};
        AssetState m_State = AssetState::Unloaded;
        std::vector<UUID> m_Dependencies;
    };

    using AssetRef = std::shared_ptr<Asset>;
}