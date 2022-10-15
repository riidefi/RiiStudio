#pragma once

#include "Texture.hpp"
#include <core/3d/i3dmodel.hpp>
#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <librii/gx.h>
#include <librii/mtx/TexMtx.hpp>
#include <rsl/ArrayVector.hpp>

namespace libcube {

class Scene;
class Model;

using GCMaterialData= librii::gx::GCMaterialData;

struct IGCMaterial : public riistudio::lib3d::Material {
  virtual GCMaterialData& getMaterialData() = 0;
  virtual const GCMaterialData& getMaterialData() const = 0;

  bool isXluPass() const override final { return getMaterialData().xlu; }
  void setXluPass(bool b) override final { getMaterialData().xlu = b; }

  virtual const libcube::Model* getParent() const { return nullptr; }
  std::pair<std::string, std::string> generateShaders() const override;

  virtual kpi::ConstCollectionRange<Texture>
  getTextureSource(const libcube::Scene& scn) const;
  virtual const Texture* getTexture(const libcube::Scene& scn,
                                    const std::string& id) const = 0;

  std::string getName() const override { return getMaterialData().name; }
  void setName(const std::string& name) override {
    getMaterialData().name = name;
  }

  void setMegaState(librii::gfx::MegaState& state) const override;

  void configure(librii::gfx::PixelOcclusion occlusion,
                 std::vector<std::string>& textures) override {
    // TODO
    if (textures.empty())
      return;

    librii::gx::ConfigureSingleTex(getMaterialData(), textures[0]);
  }
};

} // namespace libcube
