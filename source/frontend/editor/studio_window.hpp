#pragma once

#include <core/common.h>
#include <core/window/window.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <string>

namespace riistudio::frontend {

class StudioWindow : public core::Window<StudioWindow, core::IWindow> {
public:
  StudioWindow(const std::string& name, bool dockspace = false);

  void setWindowFlag(u32 flag) { mFlags |= flag; }
  std::string getName() const { return mName; }
  const ImGuiWindowClass* getWindowClass() const { return &mWindowClass; }

  virtual void draw_() {}
  virtual ImGuiID buildDock(ImGuiID root_id) { return root_id; }

  void draw() override final;

private:
  std::string idIfy(std::string in);

protected:
  std::string idIfyChild(std::string in);

private:
  ImGuiWindowClass mWindowClass;
  std::string mName;
  ImGuiID mId;
  bool mbDrawDockspace = true;
  u32 mFlags = 0;
};

} // namespace riistudio::frontend
