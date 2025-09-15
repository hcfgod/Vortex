#pragma once

#include "vxpch.h"
#include "Core/Debug/ErrorCodes.h"
#include "Engine/Renderer/Shader/Shader.h"

namespace Vortex
{
	struct PipelineDesc
	{
		std::string Name = "Pipeline";
		ShaderRef ShaderProgram; // Mandatory for now
		bool DepthTest = true;
		bool DepthWrite = true;
		uint32_t DepthFunc = 1; // SetDepthStateCommand::Less
		bool Blend = false;
		uint32_t BlendSrc = 6; // SrcAlpha
		uint32_t BlendDst = 7; // OneMinusSrcAlpha
		uint32_t BlendOp = 0;  // Add
		uint32_t CullMode = 2; // Back
		uint32_t FrontFace = 1; // CCW
	};

	class Pipeline
	{
	public:
		Pipeline() = default;
		explicit Pipeline(PipelineDesc desc) : m_Desc(std::move(desc)) {}

		const PipelineDesc& GetDesc() const { return m_Desc; }
		void SetDesc(const PipelineDesc& d) { m_Desc = d; }

		Result<void> Bind(bool executeImmediate = false) const;

	private:
		PipelineDesc m_Desc{};
	};
}


