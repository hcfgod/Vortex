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
#include "Engine/Systems/RenderSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

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

	// Helper to pack RGBA color into uint32 (RGBA8)
	static inline uint32_t PackColorRGBA8(const glm::vec4& color)
	{
		uint32_t r = (uint32_t)glm::clamp((int)glm::round(color.r * 255.0f), 0, 255);
		uint32_t g = (uint32_t)glm::clamp((int)glm::round(color.g * 255.0f), 0, 255);
		uint32_t b = (uint32_t)glm::clamp((int)glm::round(color.b * 255.0f), 0, 255);
		uint32_t a = (uint32_t)glm::clamp((int)glm::round(color.a * 255.0f), 0, 255);
		return (r << 24) | (g << 16) | (b << 8) | a;
	}

	// Optimized rotation helpers - avoid expensive matrix calculations
	inline void RotateVertex2D(glm::vec3& vertex, float cosZ, float sinZ, const glm::vec2& center)
	{
		float x = vertex.x - center.x;
		float y = vertex.y - center.y;
		vertex.x = center.x + (x * cosZ - y * sinZ);
		vertex.y = center.y + (x * sinZ + y * cosZ);
	}

	// For 3D rotations, we'll handle Z-axis rotation first (most common), then apply X/Y if needed
	inline void RotateVertex3D(glm::vec3& vertex, const glm::vec3& rotation, const glm::vec3& center)
	{
		// Convert to radians once
		float rx = glm::radians(rotation.x);
		float ry = glm::radians(rotation.y);
		float rz = glm::radians(rotation.z);

		// Pre-calculate trig functions
		float cosX = std::cos(rx);
		float sinX = std::sin(rx);
		float cosY = std::cos(ry);
		float sinY = std::sin(ry);
		float cosZ = std::cos(rz);
		float sinZ = std::sin(rz);

		// Translate to origin
		float x = vertex.x - center.x;
		float y = vertex.y - center.y;
		float z = vertex.z - center.z;

		// Apply rotations in order: Y, X, Z (matching glm::yawPitchRoll)
		// Y-axis rotation (yaw)
		float tempX = x * cosY + z * sinY;
		float tempZ = -x * sinY + z * cosY;
		x = tempX;
		z = tempZ;

		// X-axis rotation (pitch)
		float tempY = y * cosX - z * sinX;
		tempZ = y * sinX + z * cosX;
		y = tempY;
		z = tempZ;

		// Z-axis rotation (roll)
		tempX = x * cosZ - y * sinZ;
		tempY = x * sinZ + y * cosZ;
		x = tempX;
		y = tempY;

		// Translate back
		vertex.x = center.x + x;
		vertex.y = center.y + y;
		vertex.z = center.z + z;
	}

	// Cached version of RotateVertex3D for better performance
	inline CachedRotation* GetCachedRotation(const glm::vec3& rotation)
	{
		if (!s_Data) return nullptr;

		// Check if rotation is zero (very common case)
		if (rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f)
			return nullptr;  // No rotation needed

		// Look for existing cached rotation
		for (auto& cached : s_Data->RotationCache)
		{
			if (cached.frameLastUsed > 0 && 
				std::abs(cached.angles.x - rotation.x) < 0.001f &&
				std::abs(cached.angles.y - rotation.y) < 0.001f &&
				std::abs(cached.angles.z - rotation.z) < 0.001f)
			{
				cached.frameLastUsed = s_Data->CurrentFrame;
				return &cached;
			}
		}

		// Find LRU slot to replace
		CachedRotation* lruSlot = &s_Data->RotationCache[0];
		for (auto& cached : s_Data->RotationCache)
		{
			if (cached.frameLastUsed == 0 || cached.frameLastUsed < lruSlot->frameLastUsed)
				lruSlot = &cached;
		}

		// Cache new rotation
		lruSlot->angles = rotation;
		lruSlot->frameLastUsed = s_Data->CurrentFrame;

		// Pre-calculate trig functions
		float rx = glm::radians(rotation.x);
		float ry = glm::radians(rotation.y);
		float rz = glm::radians(rotation.z);

		lruSlot->cosX = std::cos(rx);
		lruSlot->sinX = std::sin(rx);
		lruSlot->cosY = std::cos(ry);
		lruSlot->sinY = std::sin(ry);
		lruSlot->cosZ = std::cos(rz);
		lruSlot->sinZ = std::sin(rz);

		return lruSlot;
	}

	// Fast cached rotation using pre-computed sin/cos values
	inline void RotateVertexCached(glm::vec3& vertex, const CachedRotation* cached, const glm::vec3& center)
	{
		if (!cached) return;

		// Translate to origin
		float x = vertex.x - center.x;
		float y = vertex.y - center.y;
		float z = vertex.z - center.z;

		// Apply rotations in order: Y, X, Z (matching glm::yawPitchRoll)
		// Y-axis rotation (yaw)
		float tempX = x * cached->cosY + z * cached->sinY;
		float tempZ = -x * cached->sinY + z * cached->cosY;
		x = tempX;
		z = tempZ;

		// X-axis rotation (pitch)
		float tempY = y * cached->cosX - z * cached->sinX;
		tempZ = y * cached->sinX + z * cached->cosX;
		y = tempY;
		z = tempZ;

		// Z-axis rotation (roll)
		tempX = x * cached->cosZ - y * cached->sinZ;
		tempY = x * cached->sinZ + y * cached->cosZ;
		x = tempX;
		y = tempY;

		// Translate back
		vertex.x = center.x + x;
		vertex.y = center.y + y;
		vertex.z = center.z + z;
	}

	// Static data for Renderer2D can be defined here if needed
	void Renderer2D::Initialize()
	{
		if (s_Data) return;
		s_Data = new Renderer2DStorage();
		s_Data->QuadVA = VertexArray::Create();

		// 1) Static per-vertex quad (unit corners and texcoords)
		struct StaticVertex { glm::vec2 Corner; glm::vec2 UV; };
		StaticVertex staticVerts[4] = {
			{ {-1.0f, -1.0f}, {0.0f, 0.0f} },
			{ { 1.0f, -1.0f}, {1.0f, 0.0f} },
			{ { 1.0f,  1.0f}, {1.0f, 1.0f} },
			{ {-1.0f,  1.0f}, {0.0f, 1.0f} },
		};
		s_Data->StaticVB = VertexBuffer::Create(sizeof(staticVerts), static_cast<const void*>(staticVerts), BufferUsage::StaticDraw);
		{
			BufferLayout layout{
				{ ShaderDataType::Vec2, "aCorner" },
				{ ShaderDataType::Vec2, "aTexCoord" }
			};
			s_Data->StaticVB->SetLayout(layout);
			s_Data->QuadVA->AddVertexBuffer(s_Data->StaticVB);
		}

		// 2) Per-instance buffer (center, halfSize, color RGBA8, texIndex, rotSinCos, z)
		s_Data->InstanceVB = VertexBuffer::Create(MaxQuads * sizeof(Renderer2DStorage::QuadInstance), nullptr, BufferUsage::StreamDraw);
		{
			BufferLayout ilayout{
				{ ShaderDataType::Vec2, "iCenter" },
				{ ShaderDataType::Vec2, "iHalfSize" },
				{ ShaderDataType::UInt, "iColor" },
				{ ShaderDataType::UInt, "iTexIndex" },
				{ ShaderDataType::Vec2, "iRotSinCos" },
				{ ShaderDataType::Float, "iZ" }
			};
			ilayout.SetDivisor(1); // per-instance
			s_Data->InstanceVB->SetLayout(ilayout);
			s_Data->QuadVA->AddVertexBuffer(s_Data->InstanceVB);
		}

		// 3) Small index buffer for the unit quad
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
		s_Data->QuadIB = IndexBuffer::Create(indices, 6);
		s_Data->QuadVA->SetIndexBuffer(s_Data->QuadIB);

		// CPU-side instance buffer
		s_Data->InstanceBuffer = new Renderer2DStorage::QuadInstance[MaxQuads];
		s_Data->InstanceBufferPtr = s_Data->InstanceBuffer;
		s_Data->InstanceCount = 0;

		// White texture
		uint32_t whitePixel = 0xFFFFFFFFu;
		Texture2D::TextureCreateInfo whiteTextureInfo;
		whiteTextureInfo.Width = 1;
		whiteTextureInfo.Height = 1;
		whiteTextureInfo.Format = TextureFormat::RGBA8;
		whiteTextureInfo.InitialData = &whitePixel;
		whiteTextureInfo.InitialDataSize = sizeof(uint32_t);
		s_Data->WhiteTexture = Texture2D::Create(whiteTextureInfo);
		// Reserve slot 0 for white texture
		s_Data->TextureSlots[0] = s_Data->WhiteTexture;

		// Initialization code for 2D renderer (shaders, buffers, etc.)
		EnsureShaderLoaded();

		m_CameraSystem = SysShared<CameraSystem>();
	}

	void Renderer2D::Shutdown()
	{
		if (!s_Data) return;

		delete[] s_Data->InstanceBuffer;

		s_Data->InstanceBuffer = nullptr;
		s_Data->InstanceBufferPtr = nullptr;
		s_Data->QuadVA.reset();
		s_Data->StaticVB.reset();
		s_Data->InstanceVB.reset();
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

		// Increment frame counter for rotation cache LRU tracking
		s_Data->CurrentFrame++;

		// Cache current viewport size (FBO if set, else window)
		if (auto* rs = Sys<RenderSystem>())
		{
			s_Data->CurrentViewportSize = rs->GetCurrentViewportSize();
		}
		else
		{
			s_Data->CurrentViewportSize = glm::uvec2(0, 0);
		}

		StartNewBatch();
	}

	void Renderer2D::EndScene()
	{
		Flush();
	}

	void Renderer2D::SetPixelSnapEnabled(bool enabled)
	{
		if (!s_Data) return;
		s_Data->PixelSnapEnabled = enabled;
	}

	// Batching utilities
	void Renderer2D::Flush()
	{
		if (!s_Data) return;
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data->InstanceBufferPtr - (uint8_t*)s_Data->InstanceBuffer);
		if (dataSize == 0 || s_Data->InstanceCount == 0)
			return;

		// Upload instance data
		s_Data->InstanceVB->SetData(s_Data->InstanceBuffer, dataSize);

		// Bind shader and set uniforms (only if shader asset is loaded)
		auto& sm = GetShaderManager();
		if (!s_Data->QuadShaderHandle.IsValid() || !s_Data->QuadShaderHandle.IsLoaded())
			return;
		sm.BindShader(s_Data->QuadShaderHandle);
		sm.SetUniform("u_ViewProjection", s_Data->CurrentViewProj);
		// Pixel snapping uniforms (engine-level)
		glm::vec2 vpSize = glm::vec2((float)s_Data->CurrentViewportSize.x, (float)s_Data->CurrentViewportSize.y);
		sm.SetUniform("u_ViewportSize", vpSize);
		sm.SetUniform("u_PixelSnap", s_Data->PixelSnapEnabled ? 1 : 0);

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
		GetRenderCommandQueue().SetBlendState(true);

		// Bind VAO and draw instanced (6 indices per quad)
		s_Data->QuadVA->Bind();
		GetRenderCommandQueue().DrawIndexed(6, s_Data->InstanceCount);

		// Restore depth defaults (test/write enabled, Less) and disable blending
		GetRenderCommandQueue().SetDepthState(true, true, SetDepthStateCommand::Less);
		GetRenderCommandQueue().SetBlendState(false);

		// Stats
		s_Data->Stats.DrawCalls += 1;
		s_Data->Stats.QuadCount += s_Data->InstanceCount;

		// Reset geometry for next batch, but keep texture slots unless StartNewBatch is called
		s_Data->InstanceCount = 0;
		s_Data->InstanceBufferPtr = s_Data->InstanceBuffer;
	}

	void Renderer2D::StartNewBatch()
{
	if (!s_Data) return;
	// Reset instance pointers
	s_Data->InstanceCount = 0;
	s_Data->InstanceBufferPtr = s_Data->InstanceBuffer;
	// Reset texture slots for a fresh batch
	s_Data->TextureSlotIndex = 1;
	s_Data->TextureSlots[0] = s_Data->WhiteTexture;
}

	// DrawQuad implementations
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (!s_Data) return;
		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = position;
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(color);
		inst.TexIndex = 0u; // white texture
		inst.RotSinCos = { 1.0f, 0.0f };
		inst.Z = 0.0f;
		++s_Data->InstanceCount;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Texture2DRef& texture, const glm::vec4& tintColor)
	{
		if (!s_Data) return;
		if (!texture) { DrawQuad(position, size, tintColor); return; }

		// Find existing texture slot or assign new one
		uint32_t texIndex = 0u;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; ++i)
		{
			if (s_Data->TextureSlots[i] && s_Data->TextureSlots[i].get() == texture.get())
			{
				texIndex = i;
				break;
			}
		}
		if (texIndex == 0u)
		{
			if (s_Data->TextureSlotIndex >= MaxTextureSlots)
			{
				Flush();
				StartNewBatch();
			}
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			texIndex = s_Data->TextureSlotIndex;
			++s_Data->TextureSlotIndex;
		}

		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = position;
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(tintColor);
		inst.TexIndex = texIndex;
		inst.RotSinCos = { 1.0f, 0.0f };
		inst.Z = 0.0f;
		++s_Data->InstanceCount;
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

		// Rotated colored quad (Unity-style Euler angles in degrees)
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color)
	{
		if (!s_Data) return;
		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		float rz = glm::radians(rotation.z);
		float c = std::cos(rz), s = std::sin(rz);
		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = position;
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(color);
		inst.TexIndex = 0u;
		inst.RotSinCos = { c, s };
		inst.Z = 0.0f;
		++s_Data->InstanceCount;
	}

		// Rotated textured quad (Texture2DRef)
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const Texture2DRef& texture, const glm::vec4& tintColor)
	{
		if (!s_Data) return;
		if (!texture) { DrawQuad(position, size, rotation, tintColor); return; }

		// Find texture slot or allocate
		uint32_t texIndex = 0u;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; ++i)
		{
			if (s_Data->TextureSlots[i] && s_Data->TextureSlots[i].get() == texture.get())
			{
				texIndex = i;
				break;
			}
		}
		if (texIndex == 0u)
		{
			if (s_Data->TextureSlotIndex >= MaxTextureSlots)
			{
				Flush();
				StartNewBatch();
			}
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			texIndex = s_Data->TextureSlotIndex;
			++s_Data->TextureSlotIndex;
		}

		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		float rz = glm::radians(rotation.z);
		float c = std::cos(rz), s = std::sin(rz);
		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = position;
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(tintColor);
		inst.TexIndex = texIndex;
		inst.RotSinCos = { c, s };
		inst.Z = 0.0f;
		++s_Data->InstanceCount;
	}

	// Rotated textured quad (TextureAsset)
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec3& rotation, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor)
	{
		if (!textureAsset.IsValid() || !textureAsset.IsLoaded())
		{
			DrawQuad(position, size, rotation, tintColor);
			return;
		}
		const TextureAsset* texAsset = textureAsset.TryGet();
		if (!texAsset || !texAsset->IsReady() || !texAsset->GetTexture())
		{
			DrawQuad(position, size, rotation, tintColor);
			return;
		}
		DrawQuad(position, size, rotation, texAsset->GetTexture(), tintColor);
	}

	// 3D positioned colored quad
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (!s_Data) return;
		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = { position.x, position.y };
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(color);
		inst.TexIndex = 0u;
		inst.RotSinCos = { 1.0f, 0.0f };
		inst.Z = position.z;
		++s_Data->InstanceCount;
	}

		// 3D positioned textured quad (Texture2DRef)
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Texture2DRef& texture, const glm::vec4& tintColor)
	{
		if (!s_Data) return;
		if (!texture) { DrawQuad(position, size, tintColor); return; }

		// Find existing texture slot or assign new one
		uint32_t texIndex = 0u;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; ++i)
		{
			if (s_Data->TextureSlots[i] && s_Data->TextureSlots[i].get() == texture.get())
			{
				texIndex = i;
				break;
			}
		}
		if (texIndex == 0u)
		{
			// Need a new slot
			if (s_Data->TextureSlotIndex >= MaxTextureSlots)
			{
				Flush();
				StartNewBatch();
			}
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			texIndex = s_Data->TextureSlotIndex;
			++s_Data->TextureSlotIndex;
		}

		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = { position.x, position.y };
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(tintColor);
		inst.TexIndex = texIndex;
		inst.RotSinCos = { 1.0f, 0.0f };
		inst.Z = position.z;
		++s_Data->InstanceCount;
	}

		// 3D positioned textured quad (TextureAsset)
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor)
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

		// 3D positioned rotated colored quad (Unity-style Euler angles in degrees)
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const glm::vec4& color)
	{
		if (!s_Data) return;
		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		float rz = glm::radians(rotation.z);
		float c = std::cos(rz), s = std::sin(rz);
		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = { position.x, position.y };
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(color);
		inst.TexIndex = 0u;
		inst.RotSinCos = { c, s };
		inst.Z = position.z;
		++s_Data->InstanceCount;
	}

		// 3D positioned rotated textured quad (Texture2DRef)
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const Texture2DRef& texture, const glm::vec4& tintColor)
	{
		if (!s_Data) return;
		if (!texture) { DrawQuad(position, size, rotation, tintColor); return; }

		// Find texture slot or allocate
		uint32_t texIndex = 0u;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; ++i)
		{
			if (s_Data->TextureSlots[i] && s_Data->TextureSlots[i].get() == texture.get())
			{
				texIndex = i;
				break;
			}
		}
		if (texIndex == 0u)
		{
			if (s_Data->TextureSlotIndex >= MaxTextureSlots)
			{
				Flush();
				StartNewBatch();
			}
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			texIndex = s_Data->TextureSlotIndex;
			++s_Data->TextureSlotIndex;
		}

		if (s_Data->InstanceCount >= MaxQuads)
		{
			Flush();
			StartNewBatch();
		}

		float rz = glm::radians(rotation.z);
		float c = std::cos(rz), s = std::sin(rz);
		auto& inst = *s_Data->InstanceBufferPtr++;
		inst.Center = { position.x, position.y };
		inst.HalfSize = size * 0.5f;
		inst.ColorRGBA = PackColorRGBA8(tintColor);
		inst.TexIndex = texIndex;
		inst.RotSinCos = { c, s };
		inst.Z = position.z;
		++s_Data->InstanceCount;
	}

	// 3D positioned rotated textured quad (TextureAsset)
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec3& rotation, const AssetHandle<TextureAsset>& textureAsset, const glm::vec4& tintColor)
	{
		if (!textureAsset.IsValid() || !textureAsset.IsLoaded())
		{
			DrawQuad(position, size, rotation, tintColor);
			return;
		}
		const TextureAsset* texAsset = textureAsset.TryGet();
		if (!texAsset || !texAsset->IsReady() || !texAsset->GetTexture())
		{
			DrawQuad(position, size, rotation, tintColor);
			return;
		}
		DrawQuad(position, size, rotation, texAsset->GetTexture(), tintColor);
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