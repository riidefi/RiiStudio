#pragma once

#include <core/common.h>

namespace riistudio::frontend {

// Over some range [0, n)
//
// - User selected node i
// - User then selects node j, with shift pressed
// -> Select all nodes inbetween
//
// - User selected node i
// - User then selects node j, with ctrl pressed
// !Selected(j) -> Select(j)
// Selected(j) -> Deselect(j)
//
// NOTE: No ALT behavior yet. If SHIFT takes priority over CTRL
class ContiguousSelection {
public:
  virtual size_t NumNodes() const = 0;
  virtual bool IsActiveSelection(size_t i) const = 0;
  virtual void SetActiveSelection(std::optional<size_t> i) = 0;
  virtual void Select(size_t i) = 0;
  virtual void Deselect(size_t i) = 0;
  virtual void DeselectAllExcept(std::optional<size_t> i) = 0;
  // A folder. Prevents disjoint selections across this boundary.
  virtual bool IsSelectionBarrier(size_t i) const = 0;
  // Say a folder. Will not act as a barrier by itself, but will not attempt to
  // select.
  virtual bool Selectable(size_t i) const = 0;

  enum SelectMode {
    SELECT_NONE,
    SELECT_ARROWKEYS,
    SELECT_CLICK,
  };

  void OnUserInput(s32 index, bool shift, bool ctrl, bool wasAlreadySelected,
                   SelectMode mode) {
    if (mode == SELECT_NONE) {
      return;
    }
    if (!Selectable(index)) {
      return;
    }
    if (shift) {
      s32 activeIndex = -1;
      for (size_t i = 0; i < NumNodes(); ++i) {
        if (IsActiveSelection(i)) {
          activeIndex = i;
          break;
        }
      }
      // If there was no prior active node, SHIFT acts as CTRL
      if (activeIndex == -1) {
        shift = false;
        ctrl = true;
      } else {
        const std::size_t a = std::min(index, activeIndex);
        const std::size_t b = std::max(index, activeIndex);
        for (std::size_t i = a; i <= b; ++i) {
          if (IsSelectionBarrier(i)) {
            break;
          }
          if (Selectable(i)) {
            Select(i);
          }
        }
      }
    }
    // If the control key was pressed, we add to the selection if necessary.
    if (ctrl && !shift) {
      // CTRL acts as a toggle here
      if (wasAlreadySelected && mode == SELECT_CLICK) {
        Deselect(index);
        return;
      }
    }
    if (!ctrl && !shift) {
      DeselectAllExcept(index);
    }
    if (!IsActiveSelection(index))
      SetActiveSelection(index);
    if (!wasAlreadySelected)
      Select(index);
  }
};

} // namespace riistudio::frontend
