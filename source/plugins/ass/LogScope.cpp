#include "LogScope.hpp"

#include <vendor/assimp/DefaultLogger.hpp>

#include <assert.h>

import std.core;
import std.threading;

namespace riistudio::ass {

static std::atomic<bool> IsAnyLoggerAttached = false;

void AttachAssimpLogger(std::unique_ptr<Assimp::Logger> logger) {
  // TODO: Better approach
  if (IsAnyLoggerAttached) {
    assert(!"Only one logger can be attached!");
    exit(1);
  }

  // XXX: To prevent a memory leak, DetachAssimpLogger must be called
  Assimp::DefaultLogger::set(logger.release());
  IsAnyLoggerAttached = true;
}

void DetachAssimpLogger() {
  // TODO: Better approach
  if (!IsAnyLoggerAttached) {
    assert(!"No logger is attached!");
    exit(1);
  }

  // This calls `delete` on logger
  Assimp::DefaultLogger::set(nullptr);
  IsAnyLoggerAttached = false;
}

} // namespace riistudio::ass
