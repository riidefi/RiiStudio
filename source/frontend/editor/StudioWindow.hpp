#pragma once

#include <core/common.h>
#include <frontend/window/window.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <string>

namespace riistudio::frontend {

inline void DefaultWindowSize() {
  // Set window size to 1/4th of parent region
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextWindowSize(ImVec2{avail.x / 2, avail.y / 2}, ImGuiCond_Once);
}

class StudioWindow : public frontend::Window<StudioWindow, frontend::IWindow> {
public:
  StudioWindow(const std::string& name, bool dockspace = false);

  void setName(const std::string& name) {
    mName = name;
    mId = ImGui::GetID(name.c_str()); // TODO
  }

  void setWindowFlag(u32 flag) { mFlags |= flag; }
  std::string getName() const { return mName; }
  const ImGuiWindowClass* getWindowClass() const { return &mWindowClass; }

  virtual void draw_() {}
  virtual ImGuiID buildDock(ImGuiID root_id) { return root_id; }

  void draw() override final;

  void InheritWindowClass(frontend::IWindow* other) {
    // Inherit window class (only can dock children into a parent)
    StudioWindow* parent = dynamic_cast<StudioWindow*>(other);
    if (parent)
      ImGui::SetNextWindowClass(parent->getWindowClass());
  }

  bool Begin(const std::string& name, bool* open, u32 flags,
             frontend::IWindow* parent) {
    InheritWindowClass(parent);
    DefaultWindowSize();
    const auto id_name = idIfy(name, parent);
    return ImGui::Begin(id_name.c_str(), open, flags);
  }
  void End() { ImGui::End(); }

private:
  std::string idIfy(std::string in, frontend::IWindow* parent);

protected:
  std::string idIfyChild(std::string in);

private:
  ImGuiWindowClass mWindowClass;
  std::string mName;
  ImGuiID mId;
  bool mbDrawDockspace = true;
  u32 mFlags = 0;

  void drawDockspace();
};

} // namespace riistudio::frontend
