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
#include <cstddef>

namespace Vortex
{
static Renderer2DStorage* s_Data = nullptr;
// Define static member declared in Renderer2D.h
std::shared_ptr<CameraSystem> Renderer2D::m_CameraSystem;

// Helper to (re)bind per-instance attributes to a new base offset within the current VBO
static void RebindInstanceAttribs(uint64_t baseOffset)
{
    if (!s_Data) return;
    const uint64_t stride = sizeof(Renderer2DStorage::QuadInstance);
    const uint32_t DT_Float = static_cast<uint32_t>(DataType::Float);
    const uint32_t DT_UInt  = static_cast<uint32_t>(DataType::UnsignedInt);

    s_Data->QuadVA->Bind();
    GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), s_Data->InstanceVB->GetRendererID());

    GetRenderCommandQueue().EnableVertexAttribArray(2, true);
    GetRenderCommandQueue().VertexAttribPointer(2, 2, DT_Float, false, stride, baseOffset + offsetof(Renderer2DStorage::QuadInstance, Center));
    GetRenderCommandQueue().VertexAttribDivisor(2, 1, true);

    GetRenderCommandQueue().EnableVertexAttribArray(3, true);
    GetRenderCommandQueue().VertexAttribPointer(3, 2, DT_Float, false, stride, baseOffset + offsetof(Renderer2DStorage::QuadInstance, HalfSize));
    GetRenderCommandQueue().VertexAttribDivisor(3, 1, true);

    GetRenderCommandQueue().EnableVertexAttribArray(4, true);
    GetRenderCommandQueue().VertexAttribIPointer(4, 1, DT_UInt, stride, baseOffset + offsetof(Renderer2DStorage::QuadInstance, ColorRGBA));
    GetRenderCommandQueue().VertexAttribDivisor(4, 1, true);

    GetRenderCommandQueue().EnableVertexAttribArray(5, true);
    GetRenderCommandQueue().VertexAttribIPointer(5, 1, DT_UInt, stride, baseOffset + offsetof(Renderer2DStorage::QuadInstance, TexIndex));
    GetRenderCommandQueue().VertexAttribDivisor(5, 1, true);

    GetRenderCommandQueue().EnableVertexAttribArray(6, true);
    GetRenderCommandQueue().VertexAttribPointer(6, 2, DT_Float, false, stride, baseOffset + offsetof(Renderer2DStorage::QuadInstance, RotSinCos));
    GetRenderCommandQueue().VertexAttribDivisor(6, 1, true);

    GetRenderCommandQueue().EnableVertexAttribArray(7, true);
    GetRenderCommandQueue().VertexAttribPointer(7, 1, DT_Float, false, stride, baseOffset + offsetof(Renderer2DStorage::QuadInstance, Z));
    GetRenderCommandQueue().VertexAttribDivisor(7, 1, true);
}

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

	// Per-instance buffer (persistent-mapped ring): center, halfSize, color RGBA8, texIndex, rotSinCos, z
	s_Data->FramesInFlight = 3;
	s_Data->FrameChunkSizeBytes = sizeof(Renderer2DStorage::QuadInstance) * MaxQuads;
	s_Data->InstanceRingSizeBytes = s_Data->FrameChunkSizeBytes * s_Data->FramesInFlight;
	s_Data->FrameFences.assign(s_Data->FramesInFlight, 0ull);

	// Create buffer object
	s_Data->InstanceVB = VertexBuffer::Create(static_cast<uint32_t>(s_Data->InstanceRingSizeBytes), nullptr, BufferUsage::StreamDraw);
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

	// Allocate persistent storage and map once
	GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), s_Data->InstanceVB->GetRendererID());
	uint32_t storageFlags = ToFlags(BufferStorageFlags::MapWriteBit | BufferStorageFlags::MapPersistentBit | BufferStorageFlags::MapCoherentBit | BufferStorageFlags::DynamicStorageBit);
	GetRenderCommandQueue().BufferStorage(static_cast<uint32_t>(BufferTarget::ArrayBuffer), s_Data->InstanceRingSizeBytes, storageFlags);
	void* mappedPtr = nullptr;
	uint32_t mapAccess = ToFlags(MapBufferAccess::MapWriteBit | MapBufferAccess::MapPersistentBit | MapBufferAccess::MapCoherentBit);
	GetRenderCommandQueue().MapBufferRange(static_cast<uint32_t>(BufferTarget::ArrayBuffer), 0, s_Data->InstanceRingSizeBytes, mapAccess, &mappedPtr);
	s_Data->InstanceMappedBase = reinterpret_cast<uint8_t*>(mappedPtr);

	s_Data->InstanceBuffer = nullptr; // we write directly into mapped GPU memory
	s_Data->InstanceBufferPtr = nullptr;
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

	// Optionally unmap (not strictly required for persistent mapping during app lifetime)
	GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), s_Data->InstanceVB ? s_Data->InstanceVB->GetRendererID() : 0);
	GetRenderCommandQueue().UnmapBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer));

	s_Data->InstanceBuffer = nullptr;
	s_Data->InstanceBufferPtr = nullptr;
	s_Data->QuadVA.reset();
	s_Data->InstanceVB.reset();
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

// Advance ring chunk and rebind attribute base offsets
s_Data->CurrentFrameChunkIndex = (s_Data->CurrentFrameChunkIndex + 1) % s_Data->FramesInFlight;

// Wait for GPU to finish using this chunk (if in flight)
uint64_t& fence = s_Data->FrameFences[s_Data->CurrentFrameChunkIndex];
if (fence != 0ull)
{
    uint32_t status = 0;
    // GL_TIMEOUT_IGNORED == 0xFFFFFFFFFFFFFFFFull; use ~0ull
    GetRenderCommandQueue().ClientWaitSync(fence, 0ull, ~0ull, &status);
    GetRenderCommandQueue().DeleteSync(fence);
    fence = 0ull;
}

uint64_t baseOffset = s_Data->CurrentFrameChunkIndex * s_Data->FrameChunkSizeBytes;

// Update attribute pointers for locations 2..7 to point at new base offset
RebindInstanceAttribs(baseOffset);

// Reset write pointer and per-frame offset for this frame
s_Data->InstanceBuffer = reinterpret_cast<Renderer2DStorage::QuadInstance*>(s_Data->InstanceMappedBase + baseOffset);
s_Data->InstanceBufferPtr = s_Data->InstanceBuffer;
s_Data->InstanceCount = 0;
s_Data->FrameInstanceOffset = 0;
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
    if (s_Data->InstanceCount == 0)
        return;

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

    // Bind VAO and draw instanced (triangle strip with 4 verts)
    s_Data->QuadVA->Bind();
    // Draw starting at FrameInstanceOffset within this frame chunk
    GetRenderCommandQueue().DrawArrays(4, s_Data->InstanceCount, 0, s_Data->FrameInstanceOffset, static_cast<uint32_t>(PrimitiveTopology::TriangleStrip));

    // Restore depth defaults (test/write enabled, Less) and disable blending
    GetRenderCommandQueue().SetDepthState(true, true, SetDepthStateCommand::Less);
    GetRenderCommandQueue().SetBlendState(false);

    // Update per-frame offset to account for the submitted instances
    s_Data->FrameInstanceOffset += s_Data->InstanceCount;

    // Replace any existing fence for this frame chunk (avoid leaking GLsyncs when flushing multiple times per frame)
    if (s_Data->FrameFences[s_Data->CurrentFrameChunkIndex] != 0ull)
    {
        GetRenderCommandQueue().DeleteSync(s_Data->FrameFences[s_Data->CurrentFrameChunkIndex]);
        s_Data->FrameFences[s_Data->CurrentFrameChunkIndex] = 0ull;
    }
    uint64_t fenceHandle = 0ull;
    GetRenderCommandQueue().FenceSync(&fenceHandle);
    s_Data->FrameFences[s_Data->CurrentFrameChunkIndex] = fenceHandle;

    // Stats
    s_Data->Stats.DrawCalls += 1;
    s_Data->Stats.QuadCount += s_Data->InstanceCount;

    // Reset geometry for next batch; keep texture slots unless caller resets them
    s_Data->InstanceCount = 0;
    s_Data->InstanceBufferPtr = s_Data->InstanceBuffer + s_Data->FrameInstanceOffset;
}

void Renderer2D::StartNewBatch()
{
    if (!s_Data) return;

    // If we've exhausted this frame chunk, rotate to the next one (with sync)
    if (s_Data->FrameInstanceOffset >= MaxQuads)
    {
        const uint32_t next = (s_Data->CurrentFrameChunkIndex + 1) % s_Data->FramesInFlight;
        uint64_t& nextFence = s_Data->FrameFences[next];
        if (nextFence != 0ull)
        {
            uint32_t status = 0;
            GetRenderCommandQueue().ClientWaitSync(nextFence, 0ull, ~0ull, &status);
            GetRenderCommandQueue().DeleteSync(nextFence);
            nextFence = 0ull;
        }
        s_Data->CurrentFrameChunkIndex = next;
        const uint64_t newBase = static_cast<uint64_t>(s_Data->CurrentFrameChunkIndex) * s_Data->FrameChunkSizeBytes;
        RebindInstanceAttribs(newBase);
        s_Data->InstanceBuffer = reinterpret_cast<Renderer2DStorage::QuadInstance*>(s_Data->InstanceMappedBase + newBase);
        s_Data->FrameInstanceOffset = 0;
    }

    // Reset instance counters and pointer to next free slot within this frame chunk
    s_Data->InstanceCount = 0;
    s_Data->InstanceBufferPtr = s_Data->InstanceBuffer + s_Data->FrameInstanceOffset;

    // Reset texture slots for a fresh batch
    s_Data->TextureSlotIndex = 1;
    s_Data->TextureSlots[0] = s_Data->WhiteTexture;
}

	// DrawQuad implementations
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (!s_Data) return;
// Ensure capacity; split batches and rotate chunks as needed
if (s_Data->FrameInstanceOffset + s_Data->InstanceCount >= MaxQuads)
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

if (s_Data->FrameInstanceOffset + s_Data->InstanceCount >= MaxQuads)
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
if (s_Data->FrameInstanceOffset + s_Data->InstanceCount >= MaxQuads)
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