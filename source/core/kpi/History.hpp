#pragma once

#include "Memento.hpp"
#include <vector>
#include <memory>

namespace kpi {

class History {
public:
  void commit(const IDocumentNode& doc) {
    if (history_cursor >= 0)
      root_history.erase(root_history.begin() + history_cursor + 1,
                         root_history.end());
    root_history.push_back(setNext(
        doc, root_history.empty() ? std::make_shared<const DocumentMemento>()
                                  : root_history.back()));
    ++history_cursor;
  }
  void undo(IDocumentNode& doc) {
    if (history_cursor <= 0)
      return;
    --history_cursor;
    rollback(doc, root_history[history_cursor]);
  }
  void redo(IDocumentNode& doc) {
    if (history_cursor + 1 >= root_history.size())
      return;
    ++history_cursor;
    rollback(doc, root_history[history_cursor]);
  }
  std::size_t cursor() const { return history_cursor; }
  std::size_t size() const { return root_history.size(); }

private:
  // At the roots, we don't need persistence
  // We don't ever expose history to anyone -- only the current document
  std::vector<std::shared_ptr<const DocumentMemento>> root_history;
  int history_cursor = -1;
};

}
