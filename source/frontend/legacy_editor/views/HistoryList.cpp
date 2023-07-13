#include "HistoryList.hpp"
#include <frontend/widgets/HistoryListWidget.hpp>

#include <frontend/properties/BrresBmdPropEdit.hpp>

namespace riistudio::frontend {

template <typename T>
struct HistoryList : public StudioWindow, private HistoryListWidget {
  HistoryList(kpi::History& host, T& root, kpi::SelectionManager& sel)
      : StudioWindow("History"), mHost(host), mRoot(root), mSel(sel) {
    setClosable(false);
  }
  ~HistoryList() = default;

private:
  // StudioWindow
  void draw_() override { drawHistoryList(); }
  // HistoryListWidget
  void commit_() override { mHost.commit(mRoot, &mSel, false); }
  void undo_() override { mHost.undo(mRoot, mSel); }
  void redo_() override { mHost.redo(mRoot, mSel); }
  int cursor_() override { return mHost.cursor(); }
  int size_() override { return mHost.size(); }

  kpi::History& mHost;
  T& mRoot;
  kpi::SelectionManager& mSel;
};

std::unique_ptr<StudioWindow> MakeHistoryList(kpi::History& host,
                                              kpi::INode& root,
                                              kpi::SelectionManager& sel) {
  if (auto* g = dynamic_cast<g3d::Collection*>(&root)) {
    return std::make_unique<HistoryList<g3d::Collection>>(host, *g, sel);
  } else if (auto* g = dynamic_cast<j3d::Collection*>(&root)) {
    return std::make_unique<HistoryList<j3d::Collection>>(host, *g, sel);
  }
  return nullptr;
}

} // namespace riistudio::frontend
