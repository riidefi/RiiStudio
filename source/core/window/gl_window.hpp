#pragma once

#include <memory>
#include <vector>
#include <string_view>

struct GLFWwindow;

namespace riistudio::core {

class GLWindow
{
public:
    GLWindow(int width=1280, int height=720, const char* title = "Untitled Window");
    ~GLWindow();

    void loop();

protected:
    virtual void frameProcess() = 0;
    virtual void frameRender() = 0;
public:
    virtual void vdrop(const std::vector<std::string>& paths) {}
	virtual void vdropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len, const std::string& name) {}

	void glfwhideMouse();
	void glfwshowMouse();

	void setVsync(bool v);

#ifdef RII_BACKEND_GLFW
	GLFWwindow* getGlfwWindow() { return mGlfwWindow; }

private:
    GLFWwindow* mGlfwWindow;
#endif
public:
	void mainLoopInternal();
};

}
