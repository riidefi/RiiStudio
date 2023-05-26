#pragma once

#include <imgui/imgui.h>
#include <string>

namespace libcube::UI {

struct ErrorState {
  std::string mTitle;
  std::string mError;
  bool mCloseRequested = false;

  ErrorState(std::string&& title) : mTitle(title) {}
  void enter(std::string&& err) {
    mError = err;
    ImGui::OpenPopup(mTitle.c_str());
  }
  void exit() { mCloseRequested = true; }
  void modal() {
    if (ImGui::BeginPopupModal(mTitle.c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", mError.c_str());
      const bool keep_alive = !ImGui::Button("OK") || mCloseRequested;
      if (!keep_alive) {
        ImGui::CloseCurrentPopup();
        mError.clear();
        mCloseRequested = false;
      }
      ImGui::EndPopup();
    }
  }
};

} // namespace libcube::UI
