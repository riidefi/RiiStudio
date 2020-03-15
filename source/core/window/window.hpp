#pragma once

#include <core/common.h>

#include <vector>
#include <memory>

namespace riistudio::core {

class Window {
public:
    virtual ~Window() = default;

    virtual void draw() {}

    inline void drawChildren() {
        for (auto& child : mChildren)
            child->draw();
    }

    bool bOpen = true;
    u32 mId = 0;
    u32 parentId = 0;

    Window* mActive = nullptr;
    Window* mParent = nullptr;
    std::vector<std::unique_ptr<Window>> mChildren;

//	#ifdef RII_BACKEND_GLFW
//		GLFWwindow* mpGlfwWindow = nullptr;
//	#endif
};

} // riistudio::core
