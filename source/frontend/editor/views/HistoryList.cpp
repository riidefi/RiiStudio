#include "HistoryList.hpp"
#include <frontend/widgets/HistoryListWidget.hpp>

namespace riistudio::frontend {

struct HistoryList : public StudioWindow, private HistoryListWidget {
  HistoryList(kpi::History& host, kpi::INode& root, kpi::SelectionManager& sel)
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
  kpi::INode& mRoot;
  kpi::SelectionManager& mSel;
};

std::unique_ptr<StudioWindow> MakeHistoryList(kpi::History& host,
                                              kpi::INode& root,
                                              kpi::SelectionManager& sel) {
  return std::make_unique<HistoryList>(host, root, sel);
}

} // namespace riistudio::frontend
