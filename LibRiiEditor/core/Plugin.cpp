#include "Plugin.hpp"
#include <fa5/IconsFontAwesome5.h>
#include "ui/Window.hpp"

#include <LibCube/GX/Material.hpp>
struct TextureOutlinerConfig
{
	static std::string res_name_plural()
	{
		return "Textures";
	}
	static std::string res_name_singular()
	{
		return "Texture";
	}
	static std::string res_icon_plural()
	{
		return ICON_FA_IMAGE;
	}
	static std::string res_icon_singular()
	{
		return ICON_FA_IMAGES;
	}
};
class TextureDataSource : public OutlinerFolder<int, SelectionManager::Texture, TextureOutlinerConfig>
{
public:
	TextureDataSource() = default;
	~TextureDataSource() = default;
};
struct TextureOutliner final : public Window
{
	TextureOutliner() = default;
	~TextureOutliner() override = default;

	void draw(WindowContext* ctx) noexcept override
	{

		if (ImGui::Begin((std::string("Texture Outliner #") + std::to_string(mId)).c_str(), &bOpen))
		{
			mFilter.Draw();

			//if (ctx)
				// outliner.draw(ctx->core_resource, ctx->selectionManager, &mFilter);

			ImGui::End();
		}

	}
	ImGuiTextFilter mFilter;
};
enum class MatTab
{
	DisplaySurface,

	Max
};
static const std::array<const char*, static_cast<size_t>(MatTab::Max)> MatTabNames = {
	"Culling"
};
struct MaterialEditor final : public Window
{
	int selected = -1;
	void draw(WindowContext* ctx) noexcept override
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
							DispState d;
							d.fromCullMode(libcube::gx::CullMode::None);
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
	struct DispState
	{
		bool front, back;

		void fromCullMode(libcube::gx::CullMode c)
		{
			switch (c)
			{
			case libcube::gx::CullMode::All:
				front = back = false;
				break;
			case libcube::gx::CullMode::None:
				front = back = true;
				break;
			case libcube::gx::CullMode::Front:
				front = false;
				back = true;
				break;
			case libcube::gx::CullMode::Back:
				front = true;
				back = false;
				break;
			}
		}
	};
};


EditorWindow::EditorWindow(std::unique_ptr<pl::FileState> state)
	: mState(std::move(state))
{

	for (const auto it : mState->mInterfaces)
	{
		switch (it->mInterfaceId)
		{
		case pl::InterfaceID::TextureList:
			attachWindow(std::move(std::make_unique<TextureOutliner>()));
			break;
		case pl::InterfaceID::LibCube_GCCollection:
			attachWindow(std::move(std::make_unique<MaterialEditor>()));
		}
	}
}
void EditorWindow::draw(WindowContext* ctx) noexcept
{
	if (!ctx)
		return;

	if (ImGui::Begin("EditorWindow", &bOpen))
	{
		ImGui::Text("Interfaces");
		for (const auto& str : mState->mInterfaces)
			ImGui::Text(std::to_string(static_cast<u32>(str->mInterfaceId)).c_str());
		ImGui::End();

		// TODO: Interface handling

	}

	

	// Check for IRenderable

	// Fill in top bar, check for IExtendedBar

	// Draw childen
	processWindowQueue();
	drawWindows(ctx);
}
