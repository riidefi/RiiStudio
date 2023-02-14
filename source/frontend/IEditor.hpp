#pragma once

#include <core/common.h>
#include <rsl/Log.hpp>

namespace riistudio::frontend {

//! Defines the interface for an editor -- a UI window with ownership over a
//! piece of file data.
//!
//!
struct IEditor {
  //! For Discord RPC. Called each frame to determine if a status update is
  //! necessary.
  virtual std::string discordStatus() const {
    return "Working on unknown things";
  }
  //! @deprecated: We always know the type of an editor when we create it. It
  //! may not be valid to open a file when another is already there. This API
  //! makes little sense.
  virtual void openFile(std::span<const u8> buf, std::string_view path) {
    rsl::error("Cannot open {}", path);
  }
  //! For the main window "Save" and "Save As" functionality.
  virtual void saveAs(std::string_view path) {
    rsl::error("Cannot save to {}", path);
  }

  //
  // Give editors full control of saving. Perhaps ideally the active editor
  // would get full control over the UI options in the top-bar, avoiding the
  // need for these APIs. For now, this is a good transition point away from
  // completely abstracted Save/Save As into a more sane API.
  //

  //! @WIP: Enables the APIs below
  virtual bool implementsCustomSaving() const { return false; }

  //! The user has clicked "Save" in the host UI window.
  //!
  //! Do not silently fail. Display a dialog box if saving is not possible.
  //!
  virtual void saveButton() {}

  //! The user has clicked "Save As" in the host UI window.
  //!
  //! Do not silently fail. Display a dialog box if saving is not possible.
  //!
  virtual void saveAsButton() {}
};

} // namespace riistudio::frontend
