#pragma once

#include <librii/g3d/data/BoneData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>

namespace librii::g3d {

bool readBone(librii::g3d::BoneData& bone, oishii::BinaryReader& reader,
              u32 bone_id, s32 data_destination);

void WriteBone(NameTable& names, oishii::Writer& writer, size_t bone_start,
               const BoneData& bone, std::span<const BoneData> bones,
               u32 bone_id, const std::set<s16>& displayMatrices);

} // namespace librii::g3d
