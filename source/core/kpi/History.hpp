#pragma once

#include "Memento.hpp"
#include <memory>
#include <vector>

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
    onCommit(doc);
  }
  void undo(IDocumentNode& doc) {
    if (history_cursor <= 0)
      return;
    --history_cursor;
    onRollback(doc);
  }
  void redo(IDocumentNode& doc) {
    if (history_cursor + 1 >= root_history.size())
      return;
    ++history_cursor;
    onRollback(doc);
  }
  std::size_t cursor() const { return history_cursor; }
  std::size_t size() const { return root_history.size(); }

  struct Observer {
    virtual ~Observer() = default;
    virtual void onCommit() {}
    virtual void beforeRollback() {}
    virtual void afterRollback() {}
  };

  void addObserver(Observer* observer) { mObservers.emplace(observer); }
  void removeObserver(Observer* observer) { mObservers.erase(observer); }

private:
  // At the roots, we don't need persistence
  // We don't ever expose history to anyone -- only the current document
  std::vector<std::shared_ptr<const DocumentMemento>> root_history;
  signed history_cursor = -1;
  std::set<Observer*> mObservers;

  void onCommit(const IDocumentNode& doc) {
    for (auto& observer : mObservers)
      observer->onCommit();
  }
  void onRollback(IDocumentNode& doc) {
    for (auto& observer : mObservers)
      observer->beforeRollback();
    rollback(doc, root_history[history_cursor]);
    for (auto& observer : mObservers)
      observer->afterRollback();
  }
};

} // namespace kpi
