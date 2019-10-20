#include "EditorWindow.hpp"
#include <fa5/IconsFontAwesome5.h>
#include <LibRiiEditor/ui/Window.hpp>

#include <LibCube/GX/Material.hpp>

#include "MaterialEditor/Components/CullMode.hpp"
#include "MaterialEditor/MatEditor.hpp"
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
			attachWindow(std::move(std::make_unique<ed::MaterialEditor>()));
			break;
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
