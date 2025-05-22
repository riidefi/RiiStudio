#pragma once

#include <core/common.h>
#include <vector>

namespace riistudio::lvl {

template <typename T>
inline void CommitHistory(size_t& cursor, std::vector<T>& chain) {
  chain.resize(cursor + 1);
  cursor++;
}

template <typename T>
inline void UndoHistory(size_t& cursor, std::vector<T>& chain) {
  if (cursor <= 0)
    return;
  --cursor;
}

template <typename T>
inline void RedoHistory(size_t& cursor, std::vector<T>& chain) {
  if (cursor + 1 >= chain.size())
    return;
  ++cursor;
}

template <typename T> struct AutoHistory {
  std::vector<T> mKmpHistory;
  size_t history_cursor = 0;
  bool commit_posted = false;

  enum class RestoreStatus { AlreadyValid, Reverted };
  RestoreStatus restoreInvalidState(T& kmp) {
    if (kmp == kmp)
      return RestoreStatus::AlreadyValid;

    // KMP has entered an invalid state, likely a NaN
    // attempt to restore it

    if (mKmpHistory.empty()) {
      // There's nothing we can do. Initial state is invalid
      assert(!"KMP read from disc is invalid");

      // TODO: Consider a special error code
      return RestoreStatus::Reverted;
    }

    // Revert it
    kmp = mKmpHistory.back();

    rsl::debug("Restored KMP to backup state\n");

    assert(kmp == kmp && "Failed to restore KMP from backup state");

    return RestoreStatus::Reverted;
  }

  void update(T& kmp) {
    if (mKmpHistory.empty()) {
      assert(kmp == kmp && "Initial state is invalid");
      mKmpHistory.push_back(kmp);
      return;
    }

    if (restoreInvalidState(kmp) != RestoreStatus::AlreadyValid)
      return;

    assert(kmp == kmp && "Kmp is in an invalid state");

    if (!commit_posted && kmp != mKmpHistory[history_cursor]) {
      commit_posted = true;
    }

    if (commit_posted && !ImGui::IsAnyMouseDown()) {
      CommitHistory(history_cursor, mKmpHistory);
      mKmpHistory.push_back(kmp);
      assert(mKmpHistory.back() == kmp);
      commit_posted = false;
    }

    // TODO: Only affect active window
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z,
                        ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteAlways)) {
      UndoHistory(history_cursor, mKmpHistory);
      kmp = mKmpHistory[history_cursor];
    } else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y,
                               ImGuiInputFlags_Repeat |
                                   ImGuiInputFlags_RouteAlways)) {
      RedoHistory(history_cursor, mKmpHistory);
      kmp = mKmpHistory[history_cursor];
    }
  }
};

} // namespace riistudio::lvl
