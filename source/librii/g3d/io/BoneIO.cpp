#include "BoneIO.hpp"

#include <core/util/glm_io.hpp>

namespace librii::g3d {

bool readBone(librii::g3d::BoneData& bone, oishii::BinaryReader& reader,
              u32 bone_id, s32 data_destination) {
  reader.skip(8); // skip size and mdl offset
  bone.mName = readName(reader, data_destination);

  const u32 id = reader.read<u32>();
  if (id != bone_id) {
    return false;
  }
  bone.matrixId = reader.read<u32>();
  const auto bone_flag = reader.read<u32>();
  bone.billboardType = reader.read<u32>();
  reader.read<u32>(); // refId
  bone.mScaling << reader;
  bone.mRotation << reader;
  bone.mTranslation << reader;

  bone.mVolume.min << reader;
  bone.mVolume.max << reader;

  auto readHierarchyElement = [&]() {
    const auto ofs = reader.read<s32>();
    if (ofs == 0)
      return -1;
    // skip to id
    oishii::Jump<oishii::Whence::Set>(reader, data_destination + ofs + 12);
    return static_cast<s32>(reader.read<u32>());
  };
  bone.mParent = readHierarchyElement();
  reader.skip(12); // Skip sibling and child links -- we recompute it all
  reader.skip(2 * ((3 * 4) * sizeof(f32))); // skip matrices

  setFromFlag(bone, bone_flag);
  return true;
}

void WriteBone(NameTable& names, oishii::Writer& writer,
               const size_t& bone_start, const BoneData& bone, u32& bone_id) {
  writeNameForward(names, writer, bone_start, bone.mName);
  writer.write<u32>(bone_id++);
  writer.write<u32>(bone.matrixId);
  writer.write<u32>(computeFlag(bone));
  writer.write<u32>(bone.billboardType);
  writer.write<u32>(0); // TODO: ref
  bone.mScaling >> writer;
  bone.mRotation >> writer;
  bone.mTranslation >> writer;
  bone.mVolume.min >> writer;
  bone.mVolume.max >> writer;

  // Parent, Child, Left, Right
  writer.write<s32>(0);
  writer.write<s32>(0);
  writer.write<s32>(0);
  writer.write<s32>(0);

  writer.write<u32>(0); // user data

  // Recomputed on runtime?
  // Mtx + Inverse, 3x4 f32
  std::array<f32, 2 * 3 * 4> matrix_data{
      1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,
      1.0, -0.0, 0.0, 0.0, -0.0, 1.0, -0.0, 0.0, 0.0, -0.0, 1.0, 0.0};
  for (auto d : matrix_data)
    writer.write<f32>(d);
}

} // namespace librii::g3d
