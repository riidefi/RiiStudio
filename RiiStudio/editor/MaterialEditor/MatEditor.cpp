#include "MatEditor.hpp"
#include <array>
#include <ThirdParty/imgui/imgui.h>
#include "Components/CullMode.hpp"
#include <LibRiiEditor/core/SelectionManager.hpp>
namespace ed {

enum class MatTab
{
	DisplaySurface,
	Color,

#ifdef DEBUG
	Debug,
#endif
	Max
};
static const std::array<const char*, static_cast<size_t>(MatTab::Max)> MatTabNames = {
	"Surface Visibility",
	"Color",

#ifdef DEBUG
	"DEBUG"
#endif
};

template<void(*PredFunc)(void)>
struct Predicate
{
	~Predicate() { PredFunc(); }
};

void MaterialEditor::draw(WindowContext* ctx) noexcept
{
	Predicate<ImGui::End> g;

	if (!ImGui::Begin("MatEditor", &bOpen) || !ctx)
		return;
		
	std::vector<libcube::GCCollection::IMaterialDelegate*> mats;

	for (const auto& sel : ctx->selectionManager.mSelections)
	{
		if (sel.type == SelectionManager::Type::Material)
		{
			mats.push_back(static_cast<libcube::GCCollection::IMaterialDelegate*>(sel.object));
		}
	}

	if (mats.empty())
	{
		ImGui::Text("No material has been loaded.");
		return;
	}

	ImGui::Text("%s %s (%u)", mats.front()->getNameCStr(), mats.size() > 1 ? "..." : "", mats.size());

	if (drawLeft(mats) && selected != -1 && selected < (int)MatTab::Max)
	{
		ImGui::SameLine();
		ImGui::BeginGroup();
		drawRight(mats);
		ImGui::EndGroup();
	}
}

bool MaterialEditor::drawLeft(std::vector<libcube::GCCollection::IMaterialDelegate*>& mats)
{
	Predicate<ImGui::EndChild> g;

	if (!ImGui::BeginChild("left pane", ImVec2(150, 0), true))
		return false;
	
	for (int i = 0; i < (int)MatTab::Max; i++)
	{
		if (ImGui::Selectable(MatTabNames[i], selected == i))
			selected = i;
	}
	return true;
}
using m = libcube::GCCollection::IMaterialDelegate;
using p = m::PropertySupport;

bool MaterialEditor::drawGenInfoTab(std::vector<libcube::GCCollection::IMaterialDelegate*>& mats)
{
	auto& active_selection = *mats.back();

	const auto feature_support = [&](p::Feature f) {
		return active_selection.support[f];
	};
	const auto can_read = [&](p::Feature c) {
		return feature_support(c) != p::Coverage::Unsupported;
	};
	const auto can_write = [&](p::Feature c) {
		return feature_support(c) == p::Coverage::ReadWrite;
	};

	auto drawCullMode = [&]() {
		if (!can_read(p::Feature::CullMode))
			return;

		ed::CullMode d(active_selection.getCullMode());
		d.draw();

		if (!can_write(p::Feature::CullMode) || d.get() == active_selection.getCullMode())
			return;

		for (auto p : mats)
			p->setCullMode(d.get());
	};
	
	drawCullMode();

	if (can_read(p::Feature::GenInfo));
	{
		const auto gen = active_selection.getGenInfo();

		ImGui::Separator();
		ImGui::Text("Gen Info");
		ImGui::Text("Number of XF Color Channels: %u", gen.colorChan);
		ImGui::Text("Number of XF Texture Coordinate Generators: %u", gen.texGen);
		ImGui::Text("Number of TEV Pixel Shader Stages: %u", gen.tevStage);
		ImGui::Text("Number of BUMP Indirect (Displacement) Stages: %u", gen.indStage);
	}

	return true;
}

bool MaterialEditor::drawRight(std::vector<libcube::GCCollection::IMaterialDelegate*>& mats)
{
	Predicate<ImGui::EndChild> g;

	if (!ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
		return false;
	
	ImGui::Text(MatTabNames[selected]);
	ImGui::Separator();
	

	if (mats.empty())
	{
		ImGui::Text("No selection.");
		return false;
	}

	auto& active_selection = *mats.back();

	switch ((MatTab)selected)
	{
	case MatTab::DisplaySurface:
		drawGenInfoTab(mats);
		break;
	case MatTab::Color: {
		float t[4]{};
		ImGui::ColorEdit4("Material Color", &t[0]);
		break;
	}
#ifdef DEBUG
	case MatTab::Debug: {
		ImGui::Text("Material Delegate Registration");
		
		const auto toStr = [](p::Coverage p) -> const char* {
			switch (p)
			{
			case p::Coverage::Unsupported:
				return "Unsupported";
			case p::Coverage::Read:
				return "Read";
			case p::Coverage::ReadWrite:
				return "ReadWrite";
			default:
				return "?";
			}
		};

		ImGui::Separator();
		ImGui::Columns(2, "DebugMatFeatures");
			ImGui::Text("Feature"); ImGui::NextColumn();
			ImGui::Text("Support"); ImGui::NextColumn();
		ImGui::Separator();

		const std::array<const char*, (u32)p::Feature::Max> features = { "cullMode", "zCompLoc", "zComp", "genInfo" };
		static int selected = -1; // FIXME
		for (u32 i = 0; i < (u32)p::Feature::Max; ++i)
		{
			ImGui::Selectable(features[i], selected == i, ImGuiSelectableFlags_SpanAllColumns);
			ImGui::NextColumn();
			ImGui::Text(toStr(active_selection.support[static_cast<p::Feature>(i)]));
			ImGui::NextColumn();
		}
		
		ImGui::Columns(1);
		ImGui::Separator();

		break;
	}
#endif
	default:
		ImGui::Text("?");
		break;
	}
	return true;
}

}
