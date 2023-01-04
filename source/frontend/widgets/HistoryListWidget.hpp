#pragma once

#include <core/common.h> // for u32
#include <imcxx/Widgets.hpp>
#include <vendor/fa5/IconsFontAwesome5.h> // for ICON_FA_SAVE

namespace riistudio::frontend {

class HistoryListWidget {
public:
  void drawHistoryList();

protected:
  virtual void commit_() {}
  virtual void undo_() {}
  virtual void redo_() {}
  virtual int cursor_() { return 0; }
  virtual int size_() { return 0; }

private:
  static bool replace(std::string& str, const std::string& from,
                      const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
      return false;
    str.replace(start_pos, from.length(), to);
    return true;
  }

  static std::string IconString(const char* base, const char* icon) {
    std::string result = riistudio::translateString(base);
    replace(result, "[ICON]", (char*)icon);
    return result;
  }
};

void HistoryListWidget::drawHistoryList() {
  std::string commit = IconString("Commit [ICON]", (char*)ICON_FA_SAVE);
  if (ImGui::Button(commit.c_str())) {
    commit_();
  }

  std::string undo = IconString("Undo [ICON]", (char*)ICON_FA_UNDO);
  if (ImGui::SameLine(); ImGui::Button(undo.c_str())) {
    undo_();
  }

  std::string redo = IconString("Redo [ICON]", (char*)ICON_FA_REDO);
  if (ImGui::SameLine(); ImGui::Button(redo.c_str())) {
    redo_();
  }

  ImGui::BeginChild("Record List"_j);
  for (std::size_t i = 0; i < size_(); ++i) {
    ImGui::Text("(%s) History #%u"_j, i == cursor_() ? "X" : " ",
                static_cast<u32>(i));
  }
  ImGui::EndChild();
}

} // namespace riistudio::frontend
