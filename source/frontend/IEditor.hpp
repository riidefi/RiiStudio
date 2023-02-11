#pragma once

#include <core/common.h>
#include <rsl/Log.hpp>

namespace riistudio::frontend {

struct IEditor {
  virtual std::string discordStatus() const {
    return "Working on unknown things";
  }
  virtual void openFile(std::span<const u8> buf, std::string_view path) {
    rsl::error("Cannot open {}", path);
  }
  virtual void saveAs(std::string_view path) {
    rsl::error("Cannot save to {}", path);
  }
};

}
