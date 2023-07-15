#include "MouseHider.hpp"

#include <frontend/root.hpp> // RootWindow

namespace riistudio::frontend {
	
void MouseHider::show() { RootWindow::spInstance->showMouse(); }
void MouseHider::hide() { RootWindow::spInstance->hideMouse(); }

} // namespace namespace riistudio::frontend
