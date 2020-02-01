#pragma once

#include <imgui/imgui.h>

// Based on the DockSpaceSimple demo.

class DockSpace
{
public:
	bool draw();

	bool bOpen = true;
	bool bFullscreenPersistant = true;
	ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_None;
};
