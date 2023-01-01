#pragma once

#include <core/common.h>
#include <expected>
#include <glm/glm.hpp>                   // glm::mat4
#include <librii/gfx/MegaState.hpp>      // MegaState
#include <librii/gfx/PixelOcclusion.hpp> // PixelOcclusion

namespace librii::glhelper {
class DelegatedUBOBuilder;
}

namespace riistudio::lib3d {

struct Material;
class Scene;
class Model;

struct Polygon;
struct Material : public virtual kpi::IObject {
  virtual ~Material() = default;

  virtual std::string getName() const { return "Untitled Material"; }
  virtual void setName(const std::string& name) {}
  virtual s64 getId() const { return -1; }

  virtual bool isXluPass() const { return false; }
  virtual void setXluPass(bool b) = 0;

  virtual std::expected<std::pair<std::string, std::string>, std::string>
  generateShaders() const = 0;

  [[nodiscard]] virtual Result<librii::gfx::MegaState> setMegaState() const = 0;
  virtual void configure(librii::gfx::PixelOcclusion occlusion,
                         std::vector<std::string>& textures) = 0;

  void onUpdate() { nextGenerationId(); }
  // Necessary for kpi::set_concrete_element
  void notifyObservers() { nextGenerationId(); }
  lib3d::Material& operator=(const lib3d::Material& rhs) {
    mGenerationId = rhs.mGenerationId;
    cachedPixelShader = rhs.cachedPixelShader;
    isShaderError = rhs.isShaderError;
    shaderError = rhs.shaderError;
    applyCacheAgain = rhs.applyCacheAgain;
    return *this;
  }

  mutable std::string cachedPixelShader;
  mutable bool isShaderError = false;
  mutable std::string shaderError;
  mutable bool applyCacheAgain = false;

  // Updating this value will force a cache invalidation. However, perhaps not
  // all changes warrant a cache invalidation. If you had the base type, you
  // could hold onto a copy of the old texture to determine if said update is
  // necessary; but in that case, you wouldn't need this system anyway.
  //
  // The other issue is that it's up to the user to update the generation ID.
  // However, that is also not resolved by using the observer system.
  //
  virtual s32 getGenerationId() const { return mGenerationId; }
  virtual void nextGenerationId() { ++mGenerationId; }

  s32 mGenerationId = 0;
};

} // namespace riistudio::lib3d
