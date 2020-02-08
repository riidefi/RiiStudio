#pragma once

#include <LibCore/Applet.hpp>
#include <LibCore/ui/widgets/Dockspace.hpp>
#include <LibCore/ui/ThemeManager.hpp>
#include <RiiStudio/IStudioWindow.hpp>

#include <queue>

struct EditorWindow;

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
    
   RootWindow();
   ~RootWindow();

private:
    DockSpace mDockSpace;
	ThemeManager mTheme;
	std::queue<std::string> mDropQueue;

	void attachEditorWindow(std::unique_ptr<EditorWindow> editor);

	std::queue<std::string> mAttachEditorsQueue;


	u32 dockspace_id;
};
