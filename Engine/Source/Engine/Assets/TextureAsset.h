#pragma once

#include "Asset.h"
#include "Engine/Renderer/Texture.h"

namespace Vortex
{
    class TextureAsset : public Asset
    {
    public:
        explicit TextureAsset(const std::string& name)
            : Asset(AssetType::Texture, name) {}

        void SetTexture(const Texture2DRef& tex) { m_Texture = tex; }
        const Texture2DRef& GetTexture() const { return m_Texture; }

        bool IsReady() const { return GetState() == AssetState::Loaded && m_Texture != nullptr; }

    private:
        Texture2DRef m_Texture;
    };

    using TextureAssetRef = std::shared_ptr<TextureAsset>;
}


