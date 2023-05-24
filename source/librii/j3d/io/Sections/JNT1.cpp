#define _USE_MATH_DEFINES
#include <cmath>

#include "../Sections.hpp"
#include <core/util/glm_io.hpp>

namespace librii::j3d {

using namespace libcube;

Result<void> readJNT1(BMDOutputContext& ctx) {
  rsl::SafeReader reader(ctx.reader);
  if (!enterSection(ctx, 'JNT1'))
    return std::unexpected("Couldn't find JNT1 section");

  ScopedSection g(ctx.reader, "Joints");

  u16 size = TRY(reader.U16());

  ctx.mdl.joints.resize(size);
  ctx.jointIdLut.resize(size);
  TRY(reader.U16());

  auto ofsJointData = TRY(reader.S32());
  auto ofsRemapTable = TRY(reader.S32());
  auto ofsStringTable = TRY(reader.S32());
  reader.seekSet(g.start);

  // Compressible resources in J3D have a relocation table (necessary for
  // interop with other animations that access by index)
  // FIXME: Note and support saving identically
  reader.seekSet(g.start + ofsRemapTable);

  bool sorted = true;
  for (int i = 0; i < size; ++i) {
    ctx.jointIdLut[i] = TRY(reader.U16());

    if (ctx.jointIdLut[i] != i)
      sorted = false;
  }

  if (!sorted) {
    ctx.transaction.callback(kpi::IOMessageClass::Warning, "JNT1",
                             "The model employs joint compression."
                             "Filesize on resave may be slightly larger.");
  }

  // FIXME: unnecessary allocation of a vector.
  reader.seekSet(ofsStringTable + g.start);
  const auto nameTable = TRY(readNameTable(reader.getUnsafe()));

  int i = 0;
  for (auto& joint : ctx.mdl.joints) {
    reader.seekSet(g.start + ofsJointData + ctx.jointIdLut[i] * 0x40);
    ctx.mdl.jointIds.push_back(ctx.jointIdLut[i]); // TODO
    joint.name = nameTable[i];
    const u16 flag = TRY(reader.U16());
    joint.flag = flag & 0xf;
    joint.bbMtxType = static_cast<librii::j3d::MatrixType>(flag >> 4);
    const u8 mayaSSC = TRY(reader.U8());
    // TODO -- keep track of this fallback behavior
    joint.mayaSSC = mayaSSC == 0xff ? false : mayaSSC;
    const auto pad = TRY(reader.U8()); // pad
    (void)pad;
    EXPECT(pad == 0xff);
    joint.scale = TRY(readVec3(reader));
    joint.rotate.x = fidxToF32(TRY(reader.S16()));
    joint.rotate.y = fidxToF32(TRY(reader.S16()));
    joint.rotate.z = fidxToF32(TRY(reader.S16()));
    TRY(reader.U16());
    joint.translate = TRY(readVec3(reader));
    joint.boundingSphereRadius = TRY(reader.F32());
    joint.boundingBox.min = TRY(readVec3(reader));
    joint.boundingBox.max = TRY(readVec3(reader));
    ++i;
  }

  return {};
}

struct JNT1Node final : public oishii::Node {
  JNT1Node(const J3dModel& model) : mModel(model) {
    mId = "JNT1";
    mLinkingRestriction.alignment = 32;
  }

  struct JointData : public oishii::Node {
    JointData(const J3dModel& mdl) : mMdl(mdl) {
      mId = "JointData";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 4;
    }

    Result<void> write(oishii::Writer& writer) const noexcept {
      for (auto& jnt : mMdl.joints) {
        writer.write<u16>((jnt.flag & 0xf) |
                          (static_cast<u32>(jnt.bbMtxType) << 4));
        writer.write<u8>(jnt.mayaSSC);
        writer.write<u8>(0xff);
        jnt.scale >> writer;
        auto rotCvt = [](float x) -> s16 {
          return roundf(x * f32(0x7fff) / 180.0f);
        };
        writer.write(rotCvt(jnt.rotate.x));
        writer.write(rotCvt(jnt.rotate.y));
        writer.write(rotCvt(jnt.rotate.z));
        writer.write<u16>(0xffff);
        jnt.translate >> writer;
        writer.write<f32>(jnt.boundingSphereRadius);
        jnt.boundingBox.min >> writer;
        jnt.boundingBox.max >> writer;
      }

      return {};
    }

    const J3dModel& mMdl;
  };
  struct JointLUT : public oishii::Node {
    JointLUT(const J3dModel& mdl) : mMdl(mdl) {
      mId = "JointLUT";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 2;
    }

    Result<void> write(oishii::Writer& writer) const noexcept {
      for (u16 i = 0; i < mMdl.joints.size(); ++i)
        writer.write<u16>(i);

      return {};
    }

    const J3dModel& mMdl;
  };
  struct JointNames : public oishii::Node {
    JointNames(const J3dModel& mdl) : mMdl(mdl) {
      mId = "JointNames";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 4;
    }

    Result<void> write(oishii::Writer& writer) const noexcept {
      auto& bones = mMdl.joints;

      std::vector<std::string> names(bones.size());

      for (int i = 0; i < bones.size(); ++i)
        names[i] = mMdl.joints[i].name;
      writeNameTable(writer, names);

      return {};
    }

    const J3dModel& mMdl;
  };
  Result<void> write(oishii::Writer& writer) const noexcept override {
    writer.write<u32, oishii::EndianSelect::Big>('JNT1');
    writer.writeLink<s32>({*this}, {"SHP1"});

    writer.write<u16>(mModel.joints.size());
    writer.write<u16>(-1);

    // TODO: We don't compress joints. I haven't seen this out in the wild yet.

    writer.writeLink<s32>(oishii::Hook(*this), oishii::Hook("JointData"));
    writer.writeLink<s32>(oishii::Hook(*this), oishii::Hook("JointLUT"));
    writer.writeLink<s32>(oishii::Hook(*this), oishii::Hook("JointNames"));

    return {};
  }

  Result<void> gatherChildren(NodeDelegate& d) const noexcept override {
    d.addNode(std::make_unique<JointData>(mModel));
    d.addNode(std::make_unique<JointLUT>(mModel));
    d.addNode(std::make_unique<JointNames>(mModel));

    return {};
  }

private:
  const J3dModel& mModel;
};

std::unique_ptr<oishii::Node> makeJNT1Node(BMDExportContext& ctx) {
  return std::make_unique<JNT1Node>(ctx.mdl);
}

} // namespace librii::j3d
