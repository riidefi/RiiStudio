#include <core/common.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/io/Common.hpp>

namespace riistudio::g3d {

void writeTexture(const librii::g3d::TextureData& data, oishii::Writer& writer,
                  NameTable& names) {
  const auto [start, span] =
      HandleBlock(writer, librii::g3d::CalcTextureBlockData(data));

  librii::g3d::WriteTexture(span, data, -start,
                            RelocationToApply{names, writer, start});
}

} // namespace riistudio::g3d
