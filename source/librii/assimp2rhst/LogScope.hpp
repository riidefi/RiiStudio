#pragma once

#include <memory>
#include <vendor/assimp/Logger.hpp>

namespace librii::assimp2rhst {

void AttachAssimpLogger(std::unique_ptr<Assimp::Logger> logger);
void DetachAssimpLogger();

struct AssimpLoggerScope {
  AssimpLoggerScope(std::unique_ptr<Assimp::Logger> logger) {
    AttachAssimpLogger(std::move(logger));
  }
  ~AssimpLoggerScope() { DetachAssimpLogger(); }
};

} // namespace librii::assimp2rhst
