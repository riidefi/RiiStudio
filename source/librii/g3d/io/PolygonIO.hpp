#pragma once

#include <core/common.h>
#include <core/kpi/Plugins.hpp>
#include <librii/g3d/data/PolygonData.hpp>
#include <librii/g3d/data/VertexData.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <rsl/SafeReader.hpp>

namespace librii::g3d {

Result<void>
ReadMesh(librii::g3d::PolygonData& poly, rsl::SafeReader& reader, bool& isValid,

         const std::vector<librii::g3d::PositionBuffer>& positions,
         const std::vector<librii::g3d::NormalBuffer>& normals,
         const std::vector<librii::g3d::ColorBuffer>& colors,
         const std::vector<librii::g3d::TextureCoordinateBuffer>& texcoords,

         kpi::LightIOTransaction& transaction,
         const std::string& transaction_path,

         u32 id);

Result<void> WriteMesh(oishii::Writer& writer,
                       const librii::g3d::PolygonData& mesh,
                       // TODO: Should not need BinaryModel
                       const librii::g3d::BinaryModel& mdl,
                       const size_t& mesh_start, NameTable& names, u32 id,
                       std::bitset<8> texmtx_needed);

} // namespace librii::g3d
