#include "vxpch.h"
#include "Renderer2D.h"
#include "Engine/Systems/SystemAccessors.h"
#include "Engine/Camera/Camera.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/Shader/Shader.h"
#include "Engine/Renderer/Shader/ShaderManager.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Assets/AssetSystem.h"
#include "Engine/Assets/TextureAsset.h"

namespace Vortex
{
	static Renderer2DStorage* s_Data = nullptr;
	// Define static member declared in Renderer2D.h
	std::shared_ptr<CameraSystem> Renderer2D::m_CameraSystem;

	static void EnsureShaderLoaded()
	{
		if (!s_Data) return;
		if (s_Data->QuadShaderHandle.IsValid()) return;

		// Load via AssetSystem by name; it will find .json or .vert/.frag under Assets
		auto assets = SysShared<AssetSystem>();
		if (assets)
		{
			s_Data->QuadShaderHandle = assets->LoadAsset<ShaderAsset>("Renderer2D");
		}
	}

	// Base quad positions / texcoords
	static const glm::vec3 s_QuadVertexPositions[4] = 
	{
		{-0.5f, -0.5f, 0.0f},
		{ 0.5f, -0.5f, 0.0f},
		{ 0.5f,  0.5f, 0.0f},
		{-0.5f,  0.5f, 0.0f}
	};

	static const glm::vec2 s_QuadTexCoords[4] = 
	{
		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.0f, 1.0f}
	};

	// Static data for Renderer2D can be defined here if needed
	void Renderer2D::Initialize()
	{
		if (s_Data) return;
		s_Data = new Renderer2DStorage();
		s_Data->QuadVA = VertexArray::Create();

		s_Data->QuadVB = VertexBuffer::Create(MaxVertices * sizeof(QuadVertex), nullptr, BufferUsage::StreamDraw);
		s_Data->QuadVB->SetLayout(
		{
			{ ShaderDataType::Vec3, "aPos" },
			{ ShaderDataType::Vec4, "aColor" },
			{ ShaderDataType::Vec2, "aTexCoord" },
			{ ShaderDataType::Float, "aTexIndex" }
		});
		s_Data->QuadVA->AddVertexBuffer(s_Data->QuadVB);

		// Pre-generate large index buffer
		std::vector<uint32_t> indices(MaxIndices);
		uint32_t offset = 0;
		for (uint32_t i = 0; i < MaxIndices; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;
			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;
			offset += 4;
		}
		s_Data->QuadIB = IndexBuffer::Create(indices.data(), MaxIndices);
		s_Data->QuadVA->SetIndexBuffer(s_Data->QuadIB);

		// White texture
		uint32_t whitePixel = 0xFFFFFFFFu;
		s_Data->QuadBuffer = new QuadVertex[MaxVertices];
		Texture2D::TextureCreateInfo whiteTextureInfo;
		whiteTextureInfo.Width = 1;
		whiteTextureInfo.Height = 1;
		whiteTextureInfo.Format = TextureFormat::RGBA8;
		whiteTextureInfo.InitialData = &whitePixel;
		whiteTextureInfo.InitialDataSize = sizeof(uint32_t);
		s_Data->WhiteTexture = Texture2D::Create(whiteTextureInfo);
		// Reserve slot 0 for white texture
		s_Data->TextureSlots[0] = s_Data->WhiteTexture;

		// Set texture sampler array [0..15]
		int samplers[MaxTextureSlots];
		for (int i = 0; i < static_cast<int>(MaxTextureSlots); ++i)
			samplers[i] = i;

		// Initialization code for 2D renderer (shaders, buffers, etc.)
		EnsureShaderLoaded();

		m_CameraSystem = SysShared<CameraSystem>();
	}

	void Renderer2D::Shutdown()
	{
		if (!s_Data) return;

		delete[] s_Data->QuadBuffer;

		s_Data->QuadBuffer = nullptr;
		s_Data->QuadBufferPtr = nullptr;
		s_Data->QuadVA.reset();
		s_Data->QuadVB.reset();
		s_Data->QuadIB.reset();
		s_Data->WhiteTexture.reset();
		s_Data->QuadShaderHandle = {};

		delete s_Data;
		s_Data = nullptr;
	}

	void Renderer2D::BeginScene(const Camera& camera)
	{
		if (!s_Data) return;
		EnsureShaderLoaded();

		s_Data->CurrentViewProj = camera.GetViewProjectionMatrix();
		s_Data->QuadIndexCount = 0;
		s_Data->QuadBufferPtr = s_Data->QuadBuffer;
		// Reset texture slots for this scene
		s_Data->TextureSlotIndex = 1;
		s_Data->TextureSlots[0] = s_Data->WhiteTexture;
	}

	void Renderer2D::EndScene()
	{
		if (!s_Data) return;
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data->QuadBufferPtr - (uint8_t*)s_Data->QuadBuffer);
		if (dataSize == 0 || s_Data->QuadIndexCount == 0)
			return;

		// Upload data
		s_Data->QuadVB->SetData(s_Data->QuadBuffer, dataSize);

		// Bind shader and set uniforms (only if shader asset is loaded)
		auto& sm = GetShaderManager();
		if (!s_Data->QuadShaderHandle.IsValid() || !s_Data->QuadShaderHandle.IsLoaded())
			return;
		sm.BindShader(s_Data->QuadShaderHandle);
		sm.SetUniform("u_ViewProjection", s_Data->CurrentViewProj);

		// Bind all textures used in this batch to their slots
		for (uint32_t i = 0; i < s_Data->TextureSlotIndex; ++i)
		{
			const auto& tex = s_Data->TextureSlots[i];
			if (tex)
			{
				std::string uniformName = "u_Textures[" + std::to_string(i) + "]";
				sm.SetTexture(uniformName, tex->GetRendererID(), i);
			}
		}

		// 2D overlay: disable depth test for draw, then restore default
		GetRenderCommandQueue().SetDepthState(false, false);
		// Enable alpha blending for textured quads
		GetRenderCommandQueue().SetBlendState(true);

		// Bind VAO and draw
		s_Data->QuadVA->Bind();
		GetRenderCommandQueue().DrawIndexed(s_Data->QuadIndexCount);

		// Restore depth defaults (test/write enabled, Less) and disable blending
		GetRenderCommandQueue().SetDepthState(true, true, SetDepthStateCommand::Less);
		GetRenderCommandQueue().SetBlendState(false);

		// Stats
		s_Data->Stats.DrawCalls += 1;
		s_Data->Stats.QuadCount += s_Data->QuadIndexCount / 6;

		// Reset
		s_Data->QuadIndexCount = 0;
		s_Data->QuadBufferPtr = s_Data->QuadBuffer;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (!s_Data) return;
		if (s_Data->QuadIndexCount + 6 > MaxIndices)
		{
			EndScene();
			if (m_CameraSystem)
			{
				auto cam = m_CameraSystem->GetActiveCamera();
				if (cam) BeginScene(*cam);
			}
		}

		glm::vec3 pos0 = { position.x + s_QuadVertexPositions[0].x * size.x, position.y + s_QuadVertexPositions[0].y * size.y, 0.0f };
		glm::vec3 pos1 = { position.x + s_QuadVertexPositions[1].x * size.x, position.y + s_QuadVertexPositions[1].y * size.y, 0.0f };
		glm::vec3 pos2 = { position.x + s_QuadVertexPositions[2].x * size.x, position.y + s_QuadVertexPositions[2].y * size.y, 0.0f };
		glm::vec3 pos3 = { position.x + s_QuadVertexPositions[3].x * size.x, position.y + s_QuadVertexPositions[3].y * size.y, 0.0f };

		QuadVertex v0{ pos0, color, s_QuadTexCoords[0], 0.0f };
		QuadVertex v1{ pos1, color, s_QuadTexCoords[1], 0.0f };
		QuadVertex v2{ pos2, color, s_QuadTexCoords[2], 0.0f };
		QuadVertex v3{ pos3, color, s_QuadTexCoords[3], 0.0f };

		*s_Data->QuadBufferPtr++ = v0;
		*s_Data->QuadBufferPtr++ = v1;
		*s_Data->QuadBufferPtr++ = v2;
		*s_Data->QuadBufferPtr++ = v3;

		s_Data->QuadIndexCount += 6;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Texture2DRef& texture, const glm::vec4& tintColor)
	{
		if (!s_Data) return;
		if (!texture) { DrawQuad(position, size, tintColor); return; }

		// Find existing texture slot or assign new one
		float texIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; ++i)
		{
			if (s_Data->TextureSlots[i] && s_Data->TextureSlots[i].get() == texture.get())
			{
				texIndex = static_cast<float>(i);
				break;
			}
		}
		if (texIndex == 0.0f)
		{
			// Need a new slot
			if (s_Data->TextureSlotIndex >= MaxTextureSlots)
			{
				// Flush and begin a new batch
				EndScene();
				if (m_CameraSystem)
				{
					auto cam = m_CameraSystem->GetActiveCamera();
					if (cam) BeginScene(*cam);
				}
			}
			texIndex = static_cast<float>(s_Data->TextureSlotIndex);
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			++s_Data->TextureSlotIndex;
		}

		if (s_Data->QuadIndexCount + 6 > MaxIndices)
		{
			EndScene();
			if (m_CameraSystem)
			{
				auto cam = m_CameraSystem->GetActiveCamera();
				if (cam) BeginScene(*cam);
			}
		}

		glm::vec3 pos0 = { position.x + s_QuadVertexPositions[0].x * size.x, position.y + s_QuadVertexPositions[0].y * size.y, 0.0f };
		glm::vec3 pos1 = { position.x + s_QuadVertexPositions[1].x * size.x, position.y + s_QuadVertexPositions[1].y * size.y, 0.0f };
		glm::vec3 pos2 = { position.x + s_QuadVertexPositions[2].x * size.x, position.y + s_QuadVertexPositions[2].y * size.y, 0.0f };
		glm::vec3 pos3 = { position.x + s_QuadVertexPositions[3].x * size.x, position.y + s_QuadVertexPositions[3].y * size.y, 0.0f };

		QuadVertex v0{ pos0, tintColor, s_QuadTexCoords[0], texIndex };
		QuadVertex v1{ pos1, tintColor, s_QuadTexCoords[1], texIndex };
		QuadVertex v2{ pos2, tintColor, s_QuadTexCoords[2], texIndex };
		QuadVertex v3{ pos3, tintColor, s_QuadTexCoords[3], texIndex };

		*s_Data->QuadBufferPtr++ = v0;
		*s_Data->QuadBufferPtr++ = v1;
		*s_Data->QuadBufferPtr++ = v2;
		*s_Data->QuadBufferPtr++ = v3;

		s_Data->QuadIndexCount += 6;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor)
	{
		if (!textureAsset.IsValid() || !textureAsset.IsLoaded())
		{
			DrawQuad(position, size, tintColor);
			return;
		}
		const TextureAsset* texAsset = textureAsset.TryGet();
		if (!texAsset || !texAsset->IsReady() || !texAsset->GetTexture())
		{
			DrawQuad(position, size, tintColor);
			return;
		}
		DrawQuad(position, size, texAsset->GetTexture(), tintColor);
	}

	const Renderer2DStatistics& Renderer2D::GetStats()
	{
		static Renderer2DStatistics empty{};
		return s_Data ? s_Data->Stats : empty;
	}

	void Renderer2D::ResetStats()
	{
		if (s_Data) s_Data->Stats = {};
	}
}