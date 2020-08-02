#include "gui/asset/EditorPipeline.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Shader.hpp"
#include "gui/widget/Combo.hpp"
#include "gui/widget/FieldAsset.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/Optional.hpp"
#include "memory/MemoryChunk.hpp"

#include <imgui.h>

using namespace gui;

// Macros for the bits each field of the project asset correspond to in the dirty flags

std::shared_ptr<AssetEditor> EditorPipeline::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorPipeline>();
}

void EditorPipeline::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::Pipeline>();
	this->mShaderVertex = asset->getVertexShader();
	this->mShaderFragment = asset->getFragmentShader();
	this->mViewport = asset->getViewport();
	this->mScissor = asset->getScissor();
	this->mFrontFace = asset->getFrontFace();
	this->mBlendOperation = asset->getBlendMode().blend;
	this->mBlendWriteMask = asset->getBlendMode().writeMask.toSet(graphics::ColorComponent::ALL);
	this->mBlendWriteMaskPreviewStr = graphics::ColorComponent::toFlagString(this->mBlendWriteMask);
	this->mDescriptors = asset->getDescriptors();

	this->mAllShaderPaths = asset::AssetManager::get()->getAssetList(asset::Shader::StaticType());
}

bool renderBlendOperation(graphics::BlendMode::Operation &blend);
bool renderBlendComponent(graphics::BlendMode::Component &comp);

void EditorPipeline::renderContent()
{
	AssetEditor::renderContent();

	if (ImGui::TreeNode("Viewport"))
	{
		if (gui::FieldNumber<f32, 2>::InlineVector("Offset", this->mViewport.offset))
		{
			this->markAssetDirty(1);
		}
		if (gui::FieldNumber<f32, 2>::InlineVector("Size", this->mViewport.size))
		{
			this->markAssetDirty(1);
		}
		if (gui::FieldNumber<f32, 2>::InlineVector("Depth Range", this->mViewport.depthRange))
		{
			this->markAssetDirty(1);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Scissor"))
	{
		if (gui::FieldNumber<f32, 2>::InlineVector("Offset", this->mScissor.offset))
		{
			this->markAssetDirty(1);
		}
		if (gui::FieldNumber<f32, 2>::InlineVector("Size", this->mScissor.size))
		{
			this->markAssetDirty(1);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Blend Mode"))
	{

		// Write Mask
		if (gui::Combo<graphics::ColorComponent::Enum>::InlineMulti(
			"Write Mask", graphics::ColorComponent::ALL, this->mBlendWriteMask,
			this->mBlendWriteMaskPreviewStr, graphics::ColorComponent::to_string,
			[](graphics::ColorComponent::Enum type) { ImGui::PushID((ui32)type); }
		))
		{
			this->mBlendWriteMaskPreviewStr = graphics::ColorComponent::toFlagString(this->mBlendWriteMask);
			this->markAssetDirty(1);
		}

		if (gui::Optional<graphics::BlendMode::Operation>::Inline(
			this->mBlendOperation, "Blend", true,
			&renderBlendOperation
		))
		{
			this->markAssetDirty(1);
		}

		ImGui::TreePop();
	}

	if (gui::Combo<graphics::FrontFace::Enum>::Inline(
		"Front Face", graphics::FrontFace::ALL, this->mFrontFace,
		graphics::FrontFace::to_string,
		[](graphics::FrontFace::Enum type) { ImGui::PushID((ui32)type); }
	))
	{
		this->markAssetDirty(1);
	}

	if (gui::FieldAsset::Inline("Vertex Shader", this->mShaderVertex.path(), this->mAllShaderPaths))
	{
		this->markAssetDirty(1);
	}
	if (gui::FieldAsset::Inline("Fragment Shader", this->mShaderFragment.path(), this->mAllShaderPaths))
	{
		this->markAssetDirty(1);
	}

}

bool renderBlendOperation(graphics::BlendMode::Operation &blend)
{
	bool bChanged = false;

	// Color Blend Mode
	{
		ImGui::PushID("ColorMode");
		ImGui::Text("Color =");
		ImGui::SameLine();
		if (renderBlendComponent(blend.color))
		{
			bChanged = true;
		}
		ImGui::PopID();
	}

	// Alpha Blend Mode
	{
		ImGui::PushID("AlphaMode");
		ImGui::Text("Alpha =");
		ImGui::SameLine();
		if (renderBlendComponent(blend.alpha))
		{
			bChanged = true;
		}
		ImGui::PopID();
	}

	return bChanged;
}

bool renderBlendComponent(graphics::BlendMode::Component &comp)
{
	bool bChanged = false;
	ImGui::PushItemWidth(150);
	if (gui::Combo<graphics::BlendFactor::Enum>::Inline(
		"###src", graphics::BlendFactor::ALL, comp.srcFactor,
		graphics::BlendFactor::to_display_string,
		[](graphics::BlendFactor::Enum type) { ImGui::PushID((ui32)type); }
	)) bChanged = true;
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::PushItemWidth(50);
	if (gui::Combo<graphics::BlendOperation::Enum>::Inline(
		"###op", graphics::BlendOperation::ALL, comp.operation,
		graphics::BlendOperation::to_display_string,
		[](graphics::BlendOperation::Enum type) { ImGui::PushID((ui32)type); }
	)) bChanged = true;
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::PushItemWidth(150);
	if (gui::Combo<graphics::BlendFactor::Enum>::Inline(
		"###dst", graphics::BlendFactor::ALL, comp.dstFactor,
		graphics::BlendFactor::to_display_string,
		[](graphics::BlendFactor::Enum type) { ImGui::PushID((ui32)type); }
	)) bChanged = true;
	ImGui::PopItemWidth();
	return bChanged;
}

void EditorPipeline::saveAsset()
{
	graphics::BlendMode blendMode;
	blendMode.blend = this->mBlendOperation;
	blendMode.writeMask = utility::Flags<graphics::ColorComponent::Enum>(this->mBlendWriteMask);

	this->get<asset::Pipeline>()
		->setVertexShader(this->mShaderVertex)
		.setFragmentShader(this->mShaderFragment)
		.setViewport(this->mViewport)
		.setScissor(this->mScissor)
		.setFrontFace(this->mFrontFace)
		.setBlendMode(blendMode)
		.setDescriptors(this->mDescriptors);
	AssetEditor::saveAsset();
}