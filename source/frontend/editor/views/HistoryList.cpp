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
    : StudioWindow("History"), mHost(host), mRoot(root) {}

HistoryList::~HistoryList() {}

void HistoryList::draw_() {
  if (ImGui::Button((const char*)u8"Commit " ICON_FA_SAVE)) {
    mHost.commit(mRoot);
  }

  if (ImGui::SameLine(); ImGui::Button((const char*)u8"Undo " ICON_FA_UNDO)) {
    mHost.undo(mRoot);
  }

  if (ImGui::SameLine(); ImGui::Button((const char*)u8"Redo " ICON_FA_REDO)) {
    mHost.redo(mRoot);
  }

  ImGui::BeginChild("Record List");
  for (std::size_t i = 0; i < mHost.size(); ++i) {
    ImGui::Text("(%s) History #%u", i == mHost.cursor() ? "X" : " ",
                static_cast<u32>(i));
  }
  ImGui::EndChild();
}

std::unique_ptr<StudioWindow> MakeHistoryList(kpi::History& host,
                                              kpi::INode& root) {
  return std::make_unique<HistoryList>(host, root);
}

} // namespace riistudio::frontend
