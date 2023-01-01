#include <core/3d/gl.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/gl/EnumConverter.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <librii/mtx/TexMtx.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Material.hpp>

IMPORT_STD;

namespace libcube {

std::expected<std::pair<std::string, std::string>, std::string>
IGCMaterial::generateShaders() const {
  auto result = TRY(librii::gl::compileShader(getMaterialData(), getName()));
  if (!applyCacheAgain)
    cachedPixelShader = result.fragment + "\n\n // End of shader";
  return std::pair<std::string, std::string>{result.vertex, result.fragment};
}

Result<librii::gfx::MegaState> IGCMaterial::setMegaState() const {
  return librii::gl::translateGfxMegaState(getMaterialData());
}
} // namespace libcube
