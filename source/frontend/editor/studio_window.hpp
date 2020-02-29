#pragma once

#include <core/common.h>
#include <core/window/window.hpp>

#include <vendor/imgui/imgui.h>
#include <vendor/imgui/imgui_internal.h>

#include <string>

namespace riistudio::frontend {

class StudioWindow : public core::Window {
public:
    StudioWindow(const std::string& name, bool dockspace=false);
    void setWindowFlag(u32 flag);
	std::string getName() const;
	const ImGuiWindowClass* getWindowClass();

	void attachWindow(std::unique_ptr<StudioWindow> win) {
		mChildren.emplace_back(std::move(win));
	}

private:
    std::string idIfy(std::string in);
	void draw() override;

	ImGuiWindowClass mWindowClass;
	std::string mName;
	ImGuiID mId;
	bool mbDrawDockspace = true;
	u32 mFlags = 0;
};

}
