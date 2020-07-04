#pragma once

#include <core/kpi/History.hpp> // History
#include <core/kpi/Node2.hpp>   // INode
#include <memory>               // std::unique_ptr

namespace kpi {

//! Holds the state of a document, including a single history channel--suitable
//! for simple editors.
class Document {
public:
  Document(std::unique_ptr<INode>&& node) : mState(std::move(node)) {}
  Document() : mState(nullptr) {}
  ~Document() = default;

  void setRoot(std::unique_ptr<INode>&& node) { mState = std::move(node); }

  INode& getRoot() {
    assert(mState.get() != nullptr);
    return *mState.get();
  }
  const INode& getRoot() const {
    assert(mState.get() != nullptr);
    return *mState.get();
  }

  History& getHistory() { return mHistory; }
  const History& getHistory() const { return mHistory; }

  //! Commit the transient root to history.
  void commit() { mHistory.commit(*mState.get()); }
  //! Undo a commit
  void undo() { mHistory.undo(*mState.get()); }
  //! Redo a commit
  void redo() { mHistory.redo(*mState.get()); }
  //! Get the number of commits past the original state.
  std::size_t cursor() const { return mHistory.cursor(); }
  //! Get the total number of commits stored. With calls to `undo`, this may be
  //! greater than `cursor()`.
  std::size_t size() const { return mHistory.size(); }

private:
  std::unique_ptr<INode> mState;
  kpi::History mHistory;
};

} // namespace kpi
