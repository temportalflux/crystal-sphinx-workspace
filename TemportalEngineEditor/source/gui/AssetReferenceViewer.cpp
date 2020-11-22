#include "gui/AssetReferenceViewer.hpp"

#include "Engine.hpp"
#include "utility/StringUtils.hpp"
#include "asset/AssetManager.hpp"

#include <imgui.h>

using namespace gui;

AssetReferenceViewer::AssetReferenceViewer(std::string title) : IGui(title)
{
}

AssetReferenceViewer::~AssetReferenceViewer()
{
}

void AssetReferenceViewer::setAssetFilePath(std::filesystem::path const& absolutePath)
{
	this->mAbsolutePath = absolutePath;
}

i32 AssetReferenceViewer::getFlags() const
{
	return ImGuiWindowFlags_None;
}

void AssetReferenceViewer::renderView()
{
	auto manager = asset::AssetManager::get();
	ImGui::BeginChild("scroll-area", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (ImGui::TreeNode("Referenced By"))
	{
		auto iterRange = manager->getAssetPathsWhichReference(this->mAbsolutePath);
		for (auto iter = iterRange.first; iter != iterRange.second; ++iter)
		{
			ImGui::Text(iter->second.filename().c_str());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("References"))
	{
		auto iterRange = manager->getAssetPathsReferencedBy(this->mAbsolutePath);
		for (auto iter = iterRange.first; iter != iterRange.second; ++iter)
		{
			ImGui::Text(iter->second.filename().c_str());
		}
		ImGui::TreePop();
	}

	ImGui::EndChild();
}