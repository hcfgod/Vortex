#include "vxpch.h"
#include "Pipeline.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Renderer/RenderCommand.h"

namespace Vortex
{
	Result<void> Pipeline::Bind(bool executeImmediate) const
	{
		if (!m_Desc.ShaderProgram || !m_Desc.ShaderProgram->IsValid())
		{
			return Result<void>(ErrorCode::InvalidParameter, "Pipeline shader is invalid");
		}

		m_Desc.ShaderProgram->Bind();

		GetRenderCommandQueue().SetDepthState(m_Desc.DepthTest, m_Desc.DepthWrite,
			static_cast<SetDepthStateCommand::CompareFunction>(m_Desc.DepthFunc));
		GetRenderCommandQueue().SetBlendState(m_Desc.Blend,
			static_cast<SetBlendStateCommand::BlendFactor>(m_Desc.BlendSrc),
			static_cast<SetBlendStateCommand::BlendFactor>(m_Desc.BlendDst),
			static_cast<SetBlendStateCommand::BlendOperation>(m_Desc.BlendOp));
		GetRenderCommandQueue().SetCullState(
			static_cast<SetCullStateCommand::CullMode>(m_Desc.CullMode),
			static_cast<SetCullStateCommand::FrontFace>(m_Desc.FrontFace));

		return Result<void>();
	}
}


