// @file PreciseBMDDump.cpp
//
// @brief For generating KCL testing suite data, we dump "Jmp2Bmd" files to
// recover original triangle data -- kcl files, on their own, are quite lossy.
//

#include "PreciseBMDDump.hpp"
#include <librii/j3d/J3dIo.hpp>
#include <rsl/Expect.hpp>
#include <rsl/Format.hpp>

namespace librii::j3d {

std::expected<std::string, std::string>
PreciseBMDDump(const J3dModel& bmd, std::string_view debug_name) {
  std::string result;

  result += std::format("PreciseBMDDump of bmd {}\n\n", debug_name);

  result += std::format("# Vertex Positions: {} entries\n",
                        bmd.vertexData.pos.mData.size());
  for (auto& vtx : bmd.vertexData.pos.mData) {
    // result += std::format("v {} {} {}\n", vtx.x, vtx.y, vtx.z);
    char buf[256];
    snprintf(buf, sizeof(buf), "f %f %f %f\n", vtx.x, vtx.y, vtx.z);
    result += std::string(buf);
  }

  result +=
      std::format("# Triangles: {} shapes\n", bmd.joints[0].displays.size());
  EXPECT(bmd.joints.size() == 1);
  for (auto& dc : bmd.joints[0].displays) {
    EXPECT(dc.material < bmd.materials.size());
    EXPECT(dc.shape < bmd.shapes.size());
    result += std::format("usemtl {}\n", bmd.materials[dc.material].name);
    auto& shp = bmd.shapes[dc.shape];
    for (auto& mp : shp.mMatrixPrimitives) {
      for (auto& p : mp.mPrimitives) {
        EXPECT(p.mType == librii::gx::PrimitiveType::TriangleStrip);
        EXPECT(p.mVertices.size() == 3);
        std::array<u16, 3> verts{
            p.mVertices[0][gx::VertexAttribute::Position],
            p.mVertices[1][gx::VertexAttribute::Position],
            p.mVertices[2][gx::VertexAttribute::Position],
        };
        EXPECT(verts[0] < bmd.vertexData.pos.mData.size());
        EXPECT(verts[1] < bmd.vertexData.pos.mData.size());
        EXPECT(verts[2] < bmd.vertexData.pos.mData.size());
        // Yes, .obj is 1-indexed...
        result += std::format("f {} {} {}\n", verts[0] + 1, verts[1] + 1,
                              verts[2] + 1);
      }
    }
    result += "\n";
  }

  return result;
}

} // namespace librii::j3d
