#pragma once

#include "WindowManager.hpp"
#include "GL_Window.hpp"

class Applet : public WindowManager, public GL_Window
{
public:
	Applet(const char* name);
	~Applet();

	void frameProcess() override;
	void frameRender() override;

	virtual void drawRoot() {}
	virtual WindowContext makeWindowContext() = 0;
};
