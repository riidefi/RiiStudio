#pragma once

#include <pfd/portable-file-dialogs.h>

namespace riistudio::frontend {

enum class UnsavedProgressResult {
  Save,
  DontSave,
  CancelClose,
};

UnsavedProgressResult unsavedProgressBox() {
  auto box = pfd::message("Unsaved Files", //
                          "Do you want to save before closing?",
                          pfd::choice::yes_no_cancel, pfd::icon::warning);

  switch (box.result()) {
  case pfd::button::yes:
    return UnsavedProgressResult::Save;
  case pfd::button::no:
    return UnsavedProgressResult::DontSave;
  case pfd::button::cancel:
  default:
    return UnsavedProgressResult::CancelClose;
  }
}

} // namespace riistudio::frontend
