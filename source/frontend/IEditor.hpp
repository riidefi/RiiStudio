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
  
  //
  // Give editors full control of saving. Perhaps ideally the active editor
  // would get full control over the UI options in the top-bar, avoiding the
  // need for these APIs. For now, this is a good transition point away from
  // completely abstracted Save/Save As into a more sane API.
  //

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
