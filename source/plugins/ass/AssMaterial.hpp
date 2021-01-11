#include "AssImporter.hpp"
#include "Utility.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <unordered_map>
#include <vendor/stb_image.h>

namespace riistudio::ass {

// Actual data we can respect
enum class ImpTexType {
  Diffuse,  // Base color
  Specular, // Attenuate spec lighting or env lighting
  Ambient,  // We can't mix with amb color, but we can just add it on.
  Emissive, // We also add this on. Perhaps later in the pipeline.
  Bump,     // We use the height map as a basic bump map.
            // We do not support normal maps. Perhaps convert to Bump.
  // We do not support shininess. In setups with two env maps, we could
  // use this to attenuate the specular one. For now, though, it is
  // unsupported.
  Opacity,      // This can actually be useful. For example, combining CMPR +
                // I8.
  Displacement, // Effectively the same as bump maps, but attenuate the
                // single diffuse component.

  // Treated as second diffuse, for now
  // LightMap,     // Any second diffuse image gets moved here
  // We don't support reflection.
  // Base Color -> Diffuse
  // Normal Camera -> Unsupported
  // Emission Color -> Emissive
  // Metalness -> Ignored for now. We can tint specular to achieve this.
  // Diffuse Roughness -> Invert for specular eventually
  // AO -> ANother LightMap
  // Unknown -> Treat as diffuse, if beyond first slot, LightMap
};
struct ImpSampler {
  ImpTexType type;
  std::string path;
  u32 uv_channel;
  librii::gx::TextureWrapMode wrap;
};
struct ImpMaterial {
  std::vector<ImpSampler> samplers;
};
static void CompileMaterial(libcube::IGCMaterial& out, const ImpMaterial& in,
                            std::set<std::string>& texturesToImport) {
  // Vertex Lighting:
  // Ambient* + (Diffuse* x LightMap x DIFFUSE_LIGHT)
  // + (Specular* x SPECULAR_LIGHT) -> Replace with opacity, not attenuate

  // Env Lighting:
  // Ambient* + (Diffuse* x LightMap x DIFFUSE_TEX*(Bump)) +
  // (Specular* x SPECULAR_TEX*(Bump)) -> Replace with opacity

  // Current, lazy setup:
  // Diffuse x Diffuse.. -> Replace with opacity

  // WIP, just supply a pure color..
  auto& data = out.getMaterialData();

  if (!in.samplers.empty()) {
    data.samplers.push_back(std::make_unique<j3d::Material::J3DSamplerData>());
    data.samplers[0]->mTexture = in.samplers[0].path;
    texturesToImport.insert(in.samplers[0].path);

    data.texMatrices.push_back(std::make_unique<j3d::Material::TexMatrix>());
    data.texGens.push_back({.matrix = librii::gx::TexMatrix::TexMatrix0});
  }

  data.cullMode = librii::gx::CullMode::Back;

  librii::gx::TevStage wip;
  wip.texMap = wip.texCoord = 0;
  wip.rasOrder = librii::gx::ColorSelChanApi::color0a0;
  wip.rasSwap = 0;

  if (!in.samplers.empty()) {
    wip.colorStage.a = librii::gx::TevColorArg::zero;
    wip.colorStage.b = librii::gx::TevColorArg::texc;
    wip.colorStage.c = librii::gx::TevColorArg::rasc;
    wip.colorStage.d = librii::gx::TevColorArg::zero;

    wip.alphaStage.a = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.b = librii::gx::TevAlphaArg::texa;
    wip.alphaStage.c = librii::gx::TevAlphaArg::rasa;
    wip.alphaStage.d = librii::gx::TevAlphaArg::zero;
  } else {
    wip.colorStage.a = librii::gx::TevColorArg::rasc;
    wip.colorStage.b = librii::gx::TevColorArg::zero;
    wip.colorStage.c = librii::gx::TevColorArg::zero;
    wip.colorStage.d = librii::gx::TevColorArg::zero;

    wip.alphaStage.a = librii::gx::TevAlphaArg::rasa;
    wip.alphaStage.b = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.c = librii::gx::TevAlphaArg::zero;
    wip.alphaStage.d = librii::gx::TevAlphaArg::zero;
  }

  data.tevColors[0] = {0xaa, 0xbb, 0xcc, 0xff};
  data.shader.mStages[0] = wip;

  librii::gx::ChannelControl ctrl;
  ctrl.enabled = false;
  ctrl.Material = librii::gx::ColorSource::Vertex;
  data.chanData[0].matColor = {0xff, 0xff, 0xff, 0xff};
  data.colorChanControls.push_back(ctrl); // rgb
  data.colorChanControls.push_back(ctrl); // a
}
static std::tuple<aiString, unsigned int, aiTextureMapMode>
GetTexture(aiMaterial* pMat, int t, int j) {
  aiString path;
  // No support for anything non-uv -- already processed away by the time
  // we get it aiTextureMapping mapping;
  unsigned int uvindex = 0;
  // We don't support blend/op.
  // All the data here *could* be translated to TEV,
  // but it isn't practical.
  // ai_real blend;
  // aiTextureOp op;
  // No decal support
  u32 mapmode = aiTextureMapMode_Wrap;

  pMat->GetTexture(static_cast<aiTextureType>(t), j, &path, nullptr, &uvindex,
                   nullptr, nullptr, nullptr //(aiTextureMapMode*)&mapmode
  );
  return {path, uvindex, (aiTextureMapMode)mapmode};
}

} // namespace riistudio::ass
