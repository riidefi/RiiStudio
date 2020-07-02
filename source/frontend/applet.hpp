#pragma once

#include <plate/Platform.hpp>
#include <frontend/window/window.hpp>

namespace riistudio::frontend {

class Applet : public Window<IWindow, void>, public plate::Platform {
public:
  Applet(const std::string& name);
  ~Applet();

  void rootCalc() override;
  void rootDraw() override;
};

// TODO -- place more appropriately
void Markdown(const std::string& markdown_);

} // namespace riistudio::frontend
