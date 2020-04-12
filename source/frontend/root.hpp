#pragma once

#include <queue>
#include <string>

#include <core/applet.hpp>

#include "file_host.hpp"
#include "ThemeManager.hpp"



namespace riistudio::frontend {

class EditorWindow;

class RootWindow final : public core::Applet, FileHost {
public:
	RootWindow();
	~RootWindow();
    void draw() override;
    void onFileOpen(FileData data, OpenFilePolicy policy) override;


	void vdrop(const std::vector<std::string>& paths) override { FileHost::drop(paths); }
	void vdropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& name) override {
		FileHost::dropDirect(std::move(data), len, name);
	}
	void attachEditorWindow(std::unique_ptr<EditorWindow> editor);
	void save(const std::string& path);
	void saveAs();

private:
    u32 dockspace_id = 0;
    bool vsync = 0;
	bool bThemeEditor = false;
	float mFontGlobalScale = 0.8f;

	std::queue<std::string> mAttachEditorsQueue;
	ThemeManager mTheme;
	ThemeManager::BasicTheme mCurTheme = ThemeManager::BasicTheme::CorporateGrey;
};

}
