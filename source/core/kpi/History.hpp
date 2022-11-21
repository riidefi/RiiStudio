#pragma once

#include "Memento.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace kpi {

struct IObject;

struct SelectionManager {
  kpi::IObject* getActive() { return mActive; }
  const kpi::IObject* getActive() const { return mActive; }
  void setActive(kpi::IObject* active) { mActive = active; }

  void select(kpi::IObject* obj) { selected.insert(obj); }
  void deselect(kpi::IObject* obj) { selected.erase(obj); }

  bool isSelected(kpi::IObject* obj) const { return selected.contains(obj); }

  void onUndoRedo_ResetSelection() {
    selected = {};
    mActive = nullptr;

    for (auto& cb : mUndoRedoCbs) {
      cb.second();
    }
  }

  std::set<kpi::IObject*> selected;
  kpi::IObject* mActive = nullptr;
  std::vector<std::pair<void*, std::function<void(void)>>> mUndoRedoCbs;
};

class History {
public:
  void commit(const IMementoOriginator& doc, SelectionManager* sel = nullptr,
              bool select_reset = false) {
    if (history_cursor >= 0)
      root_history.erase(root_history.begin() + history_cursor + 1,
                         root_history.end());
    root_history.push_back(setNext(
        doc, root_history.empty() ? nullptr : root_history.back().get()));
    needs_select_reset.push_back(select_reset);
    if (select_reset && sel != nullptr) {
      sel->onUndoRedo_ResetSelection();
    }
    ++history_cursor;
  }
  void undo(IMementoOriginator& doc, SelectionManager& sel) {
    if (history_cursor <= 0)
      return;
    if (needs_select_reset[history_cursor])
      sel.onUndoRedo_ResetSelection();
    --history_cursor;
    rollbackTo(doc, history_cursor);
  }
  void redo(IMementoOriginator& doc, SelectionManager& sel) {
    if (history_cursor + 1 >= root_history.size())
      return;
    ++history_cursor;
    if (needs_select_reset[history_cursor])
      sel.onUndoRedo_ResetSelection();
    rollbackTo(doc, history_cursor);
  }
  std::size_t cursor() const { return history_cursor; }
  std::size_t size() const { return root_history.size(); }

private:
  // At the roots, we don't need persistence
  // We don't ever expose history to anyone -- only the current document
  std::vector<std::shared_ptr<const IMemento>> root_history;
  // Adding an additional item could mean selected.mActive points to a now-stale
  // object.
  std::vector<bool> needs_select_reset;
  signed history_cursor = -1;

  void rollbackTo(IMementoOriginator& doc, unsigned position) {
    rollback(doc, *root_history[position].get());
  }
};

} // namespace kpi
