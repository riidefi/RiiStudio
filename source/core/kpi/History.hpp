#pragma once

#include "Memento.hpp"
#include <memory>
#include <vector>

namespace kpi {

class History {
public:
  void commit(const IMementoOriginator& doc) {
    if (history_cursor >= 0)
      root_history.erase(root_history.begin() + history_cursor + 1,
                         root_history.end());
    root_history.push_back(setNext(
        doc, root_history.empty() ? nullptr : root_history.back().get()));
    ++history_cursor;
  }
  void undo(IMementoOriginator& doc) {
    if (history_cursor <= 0)
      return;
    --history_cursor;
    rollbackTo(doc, history_cursor);
  }
  void redo(IMementoOriginator& doc) {
    if (history_cursor + 1 >= root_history.size())
      return;
    ++history_cursor;
    rollbackTo(doc, history_cursor);
  }
  std::size_t cursor() const { return history_cursor; }
  std::size_t size() const { return root_history.size(); }

private:
  // At the roots, we don't need persistence
  // We don't ever expose history to anyone -- only the current document
  std::vector<std::shared_ptr<const IMemento>> root_history;
  signed history_cursor = -1;

  void rollbackTo(IMementoOriginator& doc, unsigned position) {
    rollback(doc, *root_history[position].get());
  }
};

} // namespace kpi
