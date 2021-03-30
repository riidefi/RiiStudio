#include <algorithm>
#include <core/3d/gl.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/gl/EnumConverter.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <librii/mtx/TexMtx.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Material.hpp>

namespace libcube {

std::pair<std::string, std::string> IGCMaterial::generateShaders() const {
  auto result = librii::gl::compileShader(getMaterialData(), getName());

  assert(result);
  if (!result) {
    return {"Invalid", "Invalid"};
  }

  if (!applyCacheAgain)
    cachedPixelShader = result->fragment + "\n\n // End of shader";
  return {result->vertex, result->fragment};
}

static glm::mat4x4 arrayToMat4x4(const std::array<f32, 16>& m) {
  return glm::mat4x4{
	m[0], m[4], m[8], m[12],
	m[1], m[5], m[9], m[13],
	m[2], m[6], m[10], m[14],
	m[3], m[7], m[11], m[15],
  };
}

glm::mat4x4 GCMaterialData::TexMatrix::compute(const glm::mat4& mdl,
                                               const glm::mat4& mvp) const {
  auto texsrt =
      librii::mtx::computeTexSrt(scale, rotate, translate, transformModel);
  return librii::mtx::computeTexMtx(mdl, mvp, texsrt, arrayToMat4x4(effectMatrix), method, option);
}
void IGCMaterial::setMegaState(librii::gfx::MegaState& state) const {
  librii::gl::translateGfxMegaState(state, getMaterialData());
}
} // namespace libcube
