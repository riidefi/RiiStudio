#pragma once

#include "PixelOcclusion.hpp"             // PixelOcclusion
#include <glm/glm.hpp>                    // glm::mat4
#include <librii/gfx/MegaState.hpp>       // MegaState
#include <map>                            // std::map<string, u32>
#include <string>                         // std::string
#include <tuple>                          // std::pair<string, string>

namespace librii::glhelper {
class DelegatedUBOBuilder;
}

namespace riistudio::lib3d {

struct Material;
class Scene;
class Model;

struct IObserver {
  virtual ~IObserver() = default;
  // TODO: Detach
  virtual void update(Material* mat) {}

  IObserver() = default;
  IObserver(const IObserver&) = delete;
  IObserver(IObserver&&) = delete;
};
struct Polygon;
struct Material : public virtual kpi::IObject {
  virtual ~Material() = default;

  virtual std::string getName() const { return "Untitled Material"; }
  virtual void setName(const std::string& name) {}
  virtual s64 getId() const { return -1; }

  virtual bool isXluPass() const { return false; }
  virtual void setXluPass(bool b) = 0;

  virtual std::pair<std::string, std::string> generateShaders() const = 0;

  virtual void setMegaState(librii::gfx::MegaState& state) const = 0;
  virtual void configure(PixelOcclusion occlusion,
                         std::vector<std::string>& textures) = 0;

  // TODO: Better system..
  void notifyObservers() {
    for (auto* it : observers) {
      it->update(this);
    }
  }
  void onUpdate() {
    // (for shader recompilation)
    notifyObservers();
  }
  lib3d::Material& operator=(const lib3d::Material& rhs) {
    // Observers intentionally omitted
    cachedPixelShader = rhs.cachedPixelShader;
    isShaderError = rhs.isShaderError;
    shaderError = rhs.shaderError;
    applyCacheAgain = rhs.applyCacheAgain;
    return *this;
  }
  mutable std::vector<IObserver*> observers;

  mutable std::string cachedPixelShader;
  mutable bool isShaderError = false;
  mutable std::string shaderError;
  mutable bool applyCacheAgain = false;
};

} // namespace riistudio::lib3d
