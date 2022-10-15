#pragma once

#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <librii/g3d/data/BoneData.hpp>

namespace librii::g3d {

bool readBone(librii::g3d::BoneData& bone, oishii::BinaryReader& reader,
              u32 bone_id, s32 data_destination);

void WriteBone(NameTable& names, oishii::Writer& writer,
               const size_t& bone_start, const BoneData& bone,
               u32& bone_id);

}
