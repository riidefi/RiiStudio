#include "EditorWindow.hpp"
#include <fa5/IconsFontAwesome5.h>
#include <LibRiiEditor/ui/Window.hpp>

#include <LibCube/GX/Material.hpp>

#include "MaterialEditor/Components/CullMode.hpp"
#include "MaterialEditor/MatEditor.hpp"

#include "BoneEditor.hpp"
#include <LibRiiEditor/ui/widgets/AbstractOutliner.hpp>


//	struct TextureOutlinerConfig
//	{
//		static std::string res_name_plural()
//		{
//			return "Textures";
//		}
//		static std::string res_name_singular()
//		{
//			return "Texture";
//		}
//		static std::string res_icon_plural()
//		{
//			return ICON_FA_IMAGE;
//		}
//		static std::string res_icon_singular()
//		{
//			return ICON_FA_IMAGES;
//		}
//	};
struct MaterialOutlinerConfig
{
	static std::string res_name_plural()
	{
		return "Materials";
	}
	static std::string res_name_singular()
	{
		return "Material";
	}
	static std::string res_icon_plural()
	{
		return ICON_FA_PAINT_BRUSH;
	}
	static std::string res_icon_singular()
	{
		return ICON_FA_PAINT_BRUSH;
	}
};

struct MaterialDelegateSampler
{
	bool empty() const noexcept
	{
		return c.getNumMaterials() == 0;
	}
	u32 size() const noexcept
	{
		return c.getNumMaterials();
	}
	const char* nameAt(u32 idx) const noexcept
	{
		return c.getMaterialDelegate(idx).getNameCStr();
	}
	void* rawAt(u32 idx) const noexcept
	{
		return &c.getMaterialDelegate(idx);
	}
	libcube::GCCollection& c;
};
struct MaterialDataSource :	
	public AbstractOutlinerFolder<MaterialDelegateSampler, SelectionManager::Material, MaterialOutlinerConfig>
{};
struct MaterialOutliner final : public Window
{
	MaterialOutliner(libcube::GCCollection& c)
		: samp{ c }
	{}
	~MaterialOutliner() override = default;

	void draw(WindowContext* ctx) noexcept override
	{

		if (ImGui::Begin((std::string("Material Outliner #") + std::to_string(mId)).c_str(), &bOpen))
		{
			mFilter.Draw();

			if (ctx)
				outliner.draw(samp, ctx->selectionManager, &mFilter);

		}
		ImGui::End();
	}

	MaterialDelegateSampler samp;
	MaterialDataSource outliner;
	ImTFilter mFilter;
};
EditorWindow::EditorWindow(std::unique_ptr<pl::FileState> state, const std::string& path)
	: mState(std::move(state)), mFilePath(path)
{
	for (pl::AbstractInterface* it : mState->mInterfaces)
	{
		switch (it->mInterfaceId)
		{
		case pl::InterfaceID::TextureList:
			// attachWindow(std::move(std::make_unique<TextureOutliner>()));
			break;
		case pl::InterfaceID::LibCube_GCCollection:
			attachWindow(std::move(std::make_unique<ed::MaterialEditor>()));
			attachWindow(std::move(std::make_unique<MaterialOutliner>(*static_cast<libcube::GCCollection*>(it))));
			attachWindow(std::move(std::make_unique<ed::BoneEditor>(*static_cast<libcube::GCCollection*>(it))));
			break;
		}
	}
}
void EditorWindow::draw(WindowContext* ctx) noexcept
{
	if (!ctx)
		return;

#ifdef DEBUG
	if (ImGui::Begin("EditorWindow", &bOpen))
	{
		ImGui::Text("Interfaces");
		for (const auto& str : mState->mInterfaces)
			ImGui::Text(std::to_string(static_cast<u32>(str->mInterfaceId)).c_str());
		// TODO: Interface handling
	}
	ImGui::End();
#endif
	// Check for IRenderable

	// Fill in top bar, check for IExtendedBar

	// Draw childen
	processWindowQueue();
	drawWindows(ctx);
}
