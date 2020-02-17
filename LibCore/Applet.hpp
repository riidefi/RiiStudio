#pragma once

#include "windows/WindowManager.hpp"
#include "windows/GLWindow.hpp"

class Applet : public WindowManager, public GLWindow
{
public:
	Applet(const char* name);
	~Applet();

	void frameProcess() override;
	void frameRender() override;

	void showMouse() override { glfwshowMouse(); }
	void hideMouse() override { glfwhideMouse(); }
};
