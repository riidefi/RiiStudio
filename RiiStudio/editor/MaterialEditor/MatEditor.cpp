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

bool MaterialEditor::drawGenInfoTab(std::vector<libcube::GCCollection::IMaterialDelegate*>& mats)
{
	auto& active_selection = *mats.back();

	const auto reg = active_selection.getRegistration();

	if (reg.cullMode != m::PropSupport::Unsupported)
	{
		ed::CullMode d(active_selection.getCullMode());
		d.draw();

		if (reg.cullMode == m::PropSupport::ReadWrite && d.get() != active_selection.getCullMode())
			for (auto p : mats)
				p->setCullMode(d.get());
	}
	
	if (reg.genInfo != m::PropSupport::Unsupported)
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
		const auto reg = active_selection.getRegistration();
		
		const auto toStr = [](m::PropSupport p) -> const char* {
			switch (p)
			{
			case m::PropSupport::Unsupported:
				return "Unsupported";
			case m::PropSupport::Read:
				return "Read";
			case m::PropSupport::ReadWrite:
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

		const std::array<const char*, 4> features = { "cullMode", "zCompLoc", "zComp", "genInfo" };
		static int selected = -1; // FIXME
		int i = 0;
		for (const auto p : features)
		{
			ImGui::Selectable(p, selected == i, ImGuiSelectableFlags_SpanAllColumns);
			ImGui::NextColumn();
			ImGui::Text(toStr([](auto m, int i) {
				switch (i) {
				case 0:
					return m.cullMode;
				case 1:
					return m.zCompLoc;
				case 2:
					return m.zComp;
				case 3:
					return m.genInfo;
				}
				}(reg, i)));
			ImGui::NextColumn();
			++i;
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
