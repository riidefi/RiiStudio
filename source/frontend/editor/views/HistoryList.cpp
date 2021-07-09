#include "HistoryList.hpp"
#include <core/common.h>                  // for u32
#include <core/util/gui.hpp>              // for ImGui::Button
#include <vendor/fa5/IconsFontAwesome5.h> // for ICON_FA_SAVE

namespace riistudio::frontend {

struct HistoryList : public StudioWindow {
  HistoryList(kpi::History& host, kpi::INode& root);
  ~HistoryList();

private:
  void draw_() override;

  kpi::History& mHost;
  kpi::INode& mRoot;
};

HistoryList::HistoryList(kpi::History& host, kpi::INode& root)
    : StudioWindow("History"), mHost(host), mRoot(root) {
  setClosable(false);
}

HistoryList::~HistoryList() {}

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

void HistoryList::draw_() {
  std::string commit = IconString("Commit [ICON]", (char*)ICON_FA_SAVE);
  if (ImGui::Button(commit.c_str())) {
    mHost.commit(mRoot);
  }

  std::string undo = IconString("Undo [ICON]", (char*)ICON_FA_UNDO);
  if (ImGui::SameLine(); ImGui::Button(undo.c_str())) {
    mHost.undo(mRoot);
  }

  std::string redo = IconString("Redo [ICON]", (char*)ICON_FA_REDO);
  if (ImGui::SameLine(); ImGui::Button(redo.c_str())) {
    mHost.redo(mRoot);
  }

  ImGui::BeginChild("Record List"_j);
  for (std::size_t i = 0; i < mHost.size(); ++i) {
    ImGui::Text("(%s) History #%u"_j, i == mHost.cursor() ? "X" : " ",
                static_cast<u32>(i));
  }
  ImGui::EndChild();
}

std::unique_ptr<StudioWindow> MakeHistoryList(kpi::History& host,
                                              kpi::INode& root) {
  return std::make_unique<HistoryList>(host, root);
}

} // namespace riistudio::frontend
