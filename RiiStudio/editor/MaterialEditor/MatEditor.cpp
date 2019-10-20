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
	"Surface Visibility"
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
	{
		ImGui::SameLine();
		ImGui::BeginGroup();
		drawRight(*ctx);
		ImGui::EndGroup();
	}
}

bool MaterialEditor::drawLeft(WindowContext& ctx)
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

bool MaterialEditor::drawRight(WindowContext& ctx)
{
	Predicate<ImGui::EndChild> g;

	if (!ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
		return false;
	
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
	return true;
}

}
