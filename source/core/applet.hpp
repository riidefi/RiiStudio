#pragma once

#include <core/window/gl_window.hpp>
#include <core/window/window.hpp>

namespace riistudio::core {

class Applet : public Window, public GLWindow {
public:
  Applet(const std::string& name);
  ~Applet();

  void frameProcess() override;
  void frameRender() override;
};

// TODO -- place more appropriately
void Markdown(const std::string& markdown_);

} // namespace riistudio::core
