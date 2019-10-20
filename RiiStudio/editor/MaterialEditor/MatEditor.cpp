#include "MatEditor.hpp"
#include <array>
#include <ThirdParty/imgui/imgui.h>
#include "Components/CullMode.hpp"

namespace ed {

enum class MatTab
{
	DisplaySurface,

	Max
};
static const std::array<const char*, static_cast<size_t>(MatTab::Max)> MatTabNames = {
	"Culling"
};

struct ImGuiBeginPredicate
{
	~ImGuiBeginPredicate() { ImGui::End(); }
};

void MaterialEditor::draw(WindowContext* ctx) noexcept
{
	ImGuiBeginPredicate g;

	if (!ImGui::Begin("MatEditor", &bOpen) || !ctx)
		return;
		
	//	std::vector<int> materials;
	//	
	//	for (const auto& sel : ctx->selectionManager.mSelections)
	//		if (sel.type == SelectionManager::Material)
	//			materials.push_back(sel.object);
	//	
	//	
	//	if (materials.empty())
	//	{
	//		ImGui::Text("No material has been loaded.");
	//		return;
	//	}

	if (drawLeft(*ctx) && selected != -1 && selected < (int)MatTab::Max)
		drawRight(*ctx);	
}

bool MaterialEditor::drawLeft(WindowContext& ctx)
{
	if (ImGui::BeginChild("left pane", ImVec2(150, 0), true))
	{
		for (int i = 0; i < (int)MatTab::Max; i++)
		{
			if (ImGui::Selectable(MatTabNames[i], selected == i))
				selected = i;
		}
		ImGui::EndChild();
		return true;
	}
	else {
		ImGui::EndChild();
		return false;
	}
}

bool MaterialEditor::drawRight(WindowContext& ctx)
{
	ImGui::SameLine();
	ImGui::BeginGroup();
	if (ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) // Leave room for 1 line below us
	{
		ImGui::Text(MatTabNames[selected]);
		ImGui::Separator();
		switch ((MatTab)selected)
		{
		case MatTab::DisplaySurface:
			ImGui::Text("Show sides of faces:");
			{
				ed::CullMode d;
				d.set(libcube::gx::CullMode::None);
				ImGui::Checkbox("Front", &d.front);
				ImGui::Checkbox("Back", &d.back);
			}
			break;
		default:
			ImGui::Text("?");
			break;
		}
		ImGui::EndChild();
	}
	ImGui::EndGroup();
}

}
