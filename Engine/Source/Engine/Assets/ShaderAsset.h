#pragma once

#include "Asset.h"
#include "Engine/Renderer/Shader/Shader.h"
#include "Engine/Renderer/Shader/ShaderReflection.h"
#include <memory>

namespace Vortex
{
    class ShaderAsset : public Asset
    {
    public:
        explicit ShaderAsset(const std::string& name)
            : Asset(AssetType::Shader, name) {}

        void SetShader(const ShaderRef& shader) { m_Shader = shader; }
        const ShaderRef& GetShader() const { return m_Shader; }

        void SetReflection(const ShaderReflectionData& r) { m_Reflection = r; }
        const ShaderReflectionData& GetReflection() const { return m_Reflection; }

        void SetIsFallback(bool v) { m_IsFallback = v; }
        bool IsFallback() const { return m_IsFallback; }

        bool IsReady() const { return GetState() == AssetState::Loaded && m_Shader && m_Shader->IsValid(); }

    private:
        ShaderRef m_Shader;
        ShaderReflectionData m_Reflection;
        bool m_IsFallback = false;
    };

    using ShaderAssetRef = std::shared_ptr<ShaderAsset>;
}