#include "EditorWindow.hpp"
#include <fa5/IconsFontAwesome5.h>
#include <LibRiiEditor/ui/Window.hpp>

#include <LibCube/GX/Material.hpp>

#include "MaterialEditor/Components/CullMode.hpp"
#include "MaterialEditor/MatEditor.hpp"

#include "BoneEditor.hpp"
#include <LibRiiEditor/ui/widgets/AbstractOutliner.hpp>
#include <RiiStudio/editor/Console.hpp>


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
	std::string nameAt(u32 idx) const noexcept
	{
		return c.getMaterial(idx).getName();
	}
	void* rawAt(u32 idx) const noexcept
	{
		return &c.getMaterial(idx);
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



struct GC_Collection_Windows : public IWindowsCollection
{
	enum class ID
	{
		MaterialEditor,
		MaterialOutliner,
		BoneEditor,

		Max
	};

	u32 getNum() override { return static_cast<u32>(ID::Max); }
	const char* getName(u32 id) override
	{
		switch (static_cast<ID>(id))
		{
		case ID::MaterialEditor:
			return "Material Editor";
		case ID::MaterialOutliner:
			return "Material Outliner";
		case ID::BoneEditor:
			return "Bone Editor";
		default:
			return "?";
		}
	}
	std::unique_ptr<Window> spawn(u32 id) override
	{
		assert(gc);
		if (!gc) return nullptr;

		switch (static_cast<ID>(id))
		{
		case ID::MaterialEditor:
			return std::make_unique<ed::MaterialEditor>();
		case ID::MaterialOutliner:
			return std::make_unique<MaterialOutliner>(*gc);
		case ID::BoneEditor:
			return std::make_unique<ed::BoneEditor>(*gc);
		default:
			return nullptr;
		}
	}

	libcube::GCCollection* gc = nullptr;

	GC_Collection_Windows(libcube::GCCollection& _gc)
		: gc(&_gc)
	{}
};

EditorWindow::EditorWindow(std::unique_ptr<pl::FileState> state, PluginFactory& factory, const std::string& path)
	: mState(std::move(state)), mFilePath(path)
{
	// TODO: Recursive
	const auto hnd = factory.lookupInfo(mState->mName.namespacedId);
	assert(hnd);
	for (int i = 0; i < hnd.getNumParents(); ++i)
	{
		if (hnd.getParent(i).getName() == "gc_collection")
		{
			libcube::GCCollection* gc = hnd.caseToImmediateParent<libcube::GCCollection>(mState.get(), "gc_collection");
			assert(gc);
			if (!gc) return;

			mWindowCollection = std::move(std::make_unique<GC_Collection_Windows>(*gc));

			for (u32 i = 0; i < mWindowCollection->getNum(); ++i)
				attachWindow(std::move(mWindowCollection->spawn(i)));
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
		//	for (const auto& str : mState->mInterfaces)
		//		ImGui::Text(std::to_string(static_cast<u32>(str->mInterfaceId)).c_str());
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
