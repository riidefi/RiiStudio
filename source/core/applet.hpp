#pragma once

#include <plate/Platform.hpp>
#include <core/window/window.hpp>

namespace riistudio::core {

class Applet : public Window<IWindow, void>, public plate::Platform {
public:
  Applet(const std::string& name);
  ~Applet();

  void rootCalc() override;
  void rootDraw() override;
};

// TODO -- place more appropriately
void Markdown(const std::string& markdown_);

} // namespace riistudio::core
