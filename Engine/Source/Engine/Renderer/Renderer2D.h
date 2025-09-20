#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <array>
#include "Engine/Assets/AssetHandle.h"
#include "Engine/Assets/ShaderAsset.h"
#include "Engine/Renderer/Texture.h"

namespace Vortex
{
	// Forward declarations
	class Camera;
	class CameraSystem;
	class VertexArray;
	class VertexBuffer;
	class IndexBuffer;
	class Shader;
	class TextureAsset;

	// Batch capacity (quads per draw call). If exceeded we flush and start
	constexpr uint32_t MaxQuads = 500000;
	constexpr uint32_t MaxVertices = MaxQuads * 4;
	constexpr uint32_t MaxIndices = MaxQuads * 6;
	constexpr uint32_t MaxTextureSlots = 32;

	struct QuadVertex 
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float     TexIndex;
	};

	struct Renderer2DStatistics 
	{
		uint32_t DrawCalls = 0;
		uint32_t QuadCount = 0;
	};

	// Cached rotation data for frequently used angles
	struct CachedRotation
	{
		glm::vec3 angles = glm::vec3(0.0f);
		float cosX, sinX, cosY, sinY, cosZ, sinZ;
		uint32_t frameLastUsed = 0;
	};

	struct Renderer2DStorage
	{
		std::shared_ptr<VertexArray>  QuadVA;
		// Static per-vertex quad (unit corners + uv)
		std::shared_ptr<VertexBuffer> StaticVB;
		// Per-instance buffer
		std::shared_ptr<VertexBuffer> InstanceVB;
		std::shared_ptr<IndexBuffer>  QuadIB;

		// CPU-side instance buffer
		struct QuadInstance
		{
			glm::vec2 Center;
			glm::vec2 HalfSize;
			uint32_t  ColorRGBA;
			uint32_t  TexIndex;
			glm::vec2 RotSinCos; // (cosZ, sinZ)
			float     Z;
		};
		QuadInstance* InstanceBuffer = nullptr;
		QuadInstance* InstanceBufferPtr = nullptr;
		uint32_t InstanceCount = 0;

		std::array<Texture2DRef, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture reserved

		Texture2DRef WhiteTexture;
		AssetHandle<ShaderAsset> QuadShaderHandle;
		std::shared_ptr<Shader>  QuadShader;

		glm::mat4 CurrentViewProj = glm::mat4(1.0f);
		glm::uvec2 CurrentViewportSize = glm::uvec2(0, 0);
		bool PixelSnapEnabled = false;

		// Rotation cache for performance optimization
		std::array<CachedRotation, 16> RotationCache;
		uint32_t CurrentFrame = 0;

		Renderer2DStatistics Stats;
	};

	class Renderer2D
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void BeginScene(const Camera& camera);
		static void EndScene();

		// Configuration
		static void SetPixelSnapEnabled(bool enabled);

		// Mid-scene batching controls
		static void Flush();          // Submit current batch without ending the scene
		static void StartNewBatch();   // Reset batch state (clears geometry and texture slots)

		// Draw Quad overloads - 2D positioning
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Texture2DRef& texture, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const Texture2DRef& texture, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor = glm::vec4(1.0f));

		// Draw Quad overloads - 3D positioning
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Texture2DRef& texture, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Texture2DRef& texture, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor = glm::vec4(1.0f));

		// Renderer2D Stats
		static const Renderer2DStatistics& GetStats();
		static void ResetStats();
	private:
		static std::shared_ptr<CameraSystem> m_CameraSystem;
	};
}