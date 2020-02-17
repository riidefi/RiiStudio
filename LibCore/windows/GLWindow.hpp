#pragma once

#include <memory>
#include <vector>
#include <string_view>

struct GLFWwindow;

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
    virtual void drop(const std::vector<std::string_view>& paths) {}

	void glfwhideMouse();
	void glfwshowMouse();

	void setVsync(bool v);
    
	GLFWwindow* getGlfwWindow() { return mGlfwWindow; }

private:
    GLFWwindow* mGlfwWindow;
};
