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
  virtual void SetActiveSelection(size_t i) = 0;
  virtual void Select(size_t i) = 0;
  virtual void Deselect(size_t i) = 0;
  virtual void DeselectAll() = 0;
  void OnUserInput(s32 index, bool shift, bool ctrl, bool wasAlreadySelected,
                   bool thereWasAClick) {
    if (shift) {
      s32 activeIndex = -1;
      for (size_t i = 0; i < NumNodes(); ++i) {
        if (IsActiveSelection(i)) {
          activeIndex = i;
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
          Select(i);
        }
      }
    }
    // If the control key was pressed, we add to the selection if necessary.
    if (ctrl && !shift) {
      // CTRL acts as a toggle here
      if (wasAlreadySelected && thereWasAClick) {
        Deselect(index);
        return;
      }
    }
    SetActiveSelection(index);
    if (!ctrl && !shift) {
      DeselectAll();
    }
    if (!wasAlreadySelected)
      Select(index);
  }
};

} // namespace riistudio::frontend
