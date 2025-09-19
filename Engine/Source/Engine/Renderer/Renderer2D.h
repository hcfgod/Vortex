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

	struct Renderer2DStorage
	{
		std::shared_ptr<VertexArray>  QuadVA;
		std::shared_ptr<VertexBuffer> QuadVB;
		std::shared_ptr<IndexBuffer>  QuadIB;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadBuffer = nullptr;          // CPU side begin
		QuadVertex* QuadBufferPtr = nullptr;       // current write ptr

		std::array<Texture2DRef, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture reserved

		Texture2DRef WhiteTexture;
		AssetHandle<ShaderAsset> QuadShaderHandle;
		std::shared_ptr<Shader>  QuadShader;

		glm::mat4 CurrentViewProj = glm::mat4(1.0f);

		Renderer2DStatistics Stats;
	};

	class Renderer2D
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void BeginScene(const Camera& camera);
		static void EndScene();

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