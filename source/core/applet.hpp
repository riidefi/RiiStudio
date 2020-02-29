#pragma once

#include <core/window/window.hpp>
#include <core/window/gl_window.hpp>

namespace riistudio::core {

class Applet : public Window, public GLWindow
{
public:
	Applet(const char* name);
	~Applet();

	void frameProcess() override;
	void frameRender() override;
};

}