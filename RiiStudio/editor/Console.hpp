#pragma once

#include <LibRiiEditor/ui/Window.hpp>
#include <imgui/imgui.h>
#include <vector>
#include <string>
#include <RiiStudio/core/RiiCore.hpp>

struct LineObject
{
	std::string body;
	int current_character=0;

	std::string getBody() { return body; }
	void setBody(std::string b) { body = b; }
	int getCurrentCharacter() { return current_character; }
	void setCurrentCharacter(int c) { current_character = c; }
};

class Console : public Window
{
public:
    void draw(WindowContext* ctx) noexcept override;

	Console(RiiCore& core);

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2);
	void    ExecCommand();
	char                  InputBuf[256];
	std::vector<std::string> items;
	std::vector<LineObject> history;
	ImGuiTextFilter       Filter;
	bool                  AutoScroll;
	bool                  ScrollToBottom;
	std::string prompt = ">>> ";

	RiiCore& mCore;
};
