#pragma once

#include <LibCore/Applet.hpp>
#include <LibCore/ui/widgets/Dockspace.hpp>
#include <LibCore/ui/ThemeManager.hpp>
#include <RiiStudio/IStudioWindow.hpp>


#include <string>
#include <queue>

namespace px {
	struct IBinarySerializer;
}
namespace oishii {
	class BinaryReader;
}
class EditorWindow;

class RootWindow final : public Applet
{
public:
	enum class OpenFilePolicy
	{
		NewEditor,
		ReplaceEditorIfMatching,
		ReplaceEditor
	};

	std::vector<std::string> fileDialogueOpen();

    void openFile(OpenFilePolicy policy = OpenFilePolicy::NewEditor);
	void openFile(const std::string& path, OpenFilePolicy policy = OpenFilePolicy::NewEditor);
	void save(const std::string& path);
	void saveAs();

   void draw(Window* ctx) noexcept override;
   void drop(const std::vector<std::string_view>& paths) override;
   void dropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& name) override;
    
   RootWindow();
   ~RootWindow();

private:
    DockSpace mDockSpace;
	ThemeManager mTheme;
	std::queue<std::string> mDropQueue;
	struct DataDrop
	{
		std::unique_ptr<uint8_t[]> mData;
		std::size_t mLen;
		std::string mPath;

		DataDrop(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& path)
			: mData(std::move(data)), mLen(len), mPath(path)
		{}
	};
	std::queue<DataDrop> mDataDropQueue;

	void attachEditorWindow(std::unique_ptr<EditorWindow> editor);

	std::queue<std::string> mAttachEditorsQueue;


	u32 dockspace_id;

	bool mVsync = true;

	void openFile(DataDrop entry);
	void attachImporter(std::pair<std::string, std::unique_ptr<px::IBinarySerializer>> importer, std::unique_ptr<oishii::BinaryReader> reader, OpenFilePolicy policy);
};
