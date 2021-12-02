#pragma once

#include <memory>
#include <vendor/assimp/Logger.hpp>

namespace riistudio::ass {

void AttachAssimpLogger(std::unique_ptr<Assimp::Logger> logger);
void DetachAssimpLogger();

struct AssimpLoggerScope {
  AssimpLoggerScope(std::unique_ptr<Assimp::Logger> logger) {
    AttachAssimpLogger(std::move(logger));
  }
  ~AssimpLoggerScope() { DetachAssimpLogger(); }
};

} // namespace riistudio::ass
