#pragma once

#include "Core/Debug/ErrorCodes.h"
#include "Engine/Renderer/RenderCommandQueue.h"

namespace Vortex
{
	struct RenderPassDesc
	{
		std::string Name = "UnnamedPass";
		uint32_t ClearFlags = 0; // Use ClearCommand::ClearFlags
		glm::vec4 ClearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
		float ClearDepth = 1.0f;
		int32_t ClearStencil = 0;
		bool PushDebugGroup = true;
	};

	class RenderPass
	{
	public:
		RenderPass() = default;
		explicit RenderPass(RenderPassDesc desc) : m_Desc(std::move(desc)) {}

		void SetDesc(const RenderPassDesc& desc) { m_Desc = desc; }
		const RenderPassDesc& GetDesc() const { return m_Desc; }

		void Begin(bool executeImmediate = false)
		{
			if (m_Active) return;
			m_Active = true;
			if (m_Desc.PushDebugGroup)
			{
				GetRenderCommandQueue().PushDebugGroup(m_Desc.Name, executeImmediate);
			}
			
			if (m_Desc.ClearFlags != 0)
			{
				GetRenderCommandQueue().Clear(m_Desc.ClearFlags, m_Desc.ClearColor, m_Desc.ClearDepth, m_Desc.ClearStencil, executeImmediate);
			}
		}

		void End(bool executeImmediate = false)
		{
			if (!m_Active) return;
			if (m_Desc.PushDebugGroup)
			{
				GetRenderCommandQueue().PopDebugGroup(executeImmediate);
			}
			m_Active = false;
		}

		bool IsActive() const { return m_Active; }

	private:
		RenderPassDesc m_Desc{};
		bool m_Active = false;
	};

	class ScopedRenderPass
	{
	public:
		explicit ScopedRenderPass(RenderPass& pass, bool executeImmediate = false)
			: m_Pass(pass), m_Immediate(executeImmediate)
		{
			m_Pass.Begin(m_Immediate);
		}

		~ScopedRenderPass()
		{
			m_Pass.End(m_Immediate);
		}

		ScopedRenderPass(const ScopedRenderPass&) = delete;
		ScopedRenderPass& operator=(const ScopedRenderPass&) = delete;

	private:
		RenderPass& m_Pass;
		bool m_Immediate = false;
	};
}