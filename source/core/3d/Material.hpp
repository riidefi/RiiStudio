#pragma once

#include "MegaState.hpp"                   // MegaState
#include "PixelOcclusion.hpp"              // PixelOcclusion
#include <core/3d/renderer/UBOBuilder.hpp> // DelegatedUBOBuilder
#include <glm/glm.hpp>                     // glm::mat4
#include <map>                             // std::map<string, u32>
#include <string>                          // std::string
#include <tuple>                           // std::pair<string, string>

namespace riistudio::lib3d {

struct Material;
class Scene;


struct IObserver {
  virtual ~IObserver() = default;
  // TODO: Detach
  virtual void update(Material* mat) {}
};
struct Polygon;
struct Material {
  virtual ~Material() = default;

  virtual std::string getName() const { return "Untitled Material"; }
  virtual void setName(const std::string& name) {}
  virtual s64 getId() const { return -1; }

  virtual bool isXluPass() const { return false; }
  virtual void setXluPass(bool b) = 0;

  virtual std::pair<std::string, std::string> generateShaders() const = 0;
  // TODO: Interdependency
  virtual void generateUniforms(DelegatedUBOBuilder& builder,
                                const glm::mat4& M, const glm::mat4& V,
                                const glm::mat4& P, u32 shaderId,
                                const std::map<std::string, u32>& texIdMap,
                                const Polygon& poly,
                                const Scene& scene) const = 0;
  virtual void
  genSamplUniforms(u32 shaderId,
                   const std::map<std::string, u32>& texIdMap) const = 0;
  virtual void onSplice(DelegatedUBOBuilder& builder, const Polygon& poly,
                        u32 id) const {}
  virtual void setMegaState(MegaState& state) const = 0;
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
