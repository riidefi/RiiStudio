#include "Importer.hpp"
#include "Utility.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <unordered_map>
#include <vendor/stb_image.h>

namespace riistudio::assimp {


struct ImpSampler {
  std::string path;
  u32 uv_channel;
  librii::gx::TextureWrapMode wrap;
};
struct ImpMaterial {
  std::vector<ImpSampler> samplers;
};
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

} // namespace riistudio::assimp
