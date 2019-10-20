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
void MaterialEditor::draw(WindowContext* ctx) noexcept
{
	if (ImGui::Begin("MatEditor", &bOpen))
	{
		if (ctx)
		{
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
			//		ImGui::End();
			//		return;
			//	}

			// Draw left page

			if (ImGui::BeginChild("left pane", ImVec2(150, 0), true))
			{
				for (int i = 0; i < (int)MatTab::Max; i++)
				{
					if (ImGui::Selectable(MatTabNames[i], selected == i))
						selected = i;
				}
				ImGui::EndChild();
			}
			else {
				ImGui::EndChild();
				ImGui::End();
				return;
			}
			if (selected == -1 || selected >= (int)MatTab::Max)
			{
				ImGui::End();
				return;
			}
			// right

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
	ImGui::End();
}
}
