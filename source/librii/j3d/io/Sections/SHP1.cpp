#include "../Sections.hpp"
#include <core/util/glm_io.hpp>
#include <librii/gpu/DLMesh.hpp>

namespace librii::j3d {

using namespace libcube;

Result<void> readSHP1(BMDOutputContext& ctx) {
  rsl::SafeReader reader(ctx.reader);

  if (!enterSection(ctx, 'SHP1'))
    return std::unexpected("Failed to find SHP1 section");

  ScopedSection g(reader.getUnsafe(), "Shapes");

  u16 size = TRY(reader.U16());
  ctx.mdl.shapes.resize(size);
  ctx.shapeIdLut.resize(size);
  TRY(reader.U16());

  const auto [ofsShapeData, ofsShapeLut, ofsStringTable,
              // Describes the vertex buffers: GX_VA_POS, GX_INDEX16
              // (We can get this from VTX1)
              ofsVcdList, // "attr table"

              // DRW indices
              ofsDrwIndices,

              // Just a display list
              ofsDL, // "prim data"

              // struct MtxData {
              // s16 current_matrix; // -1 for singlebound
              // u16 matrix_list_size; // Vector of matrix indices; limited by
              // hardware int matrix_list_start;
              // }
              // Each sampled by matrix primitives
              ofsMtxData, // "mtx data"

              // size + offset?
              // matrix primitive splits
              ofsMtxPrimAccessor // "mtx grp"
  ] = reader.getUnsafe().readX<s32, 8>();
  reader.seekSet(g.start);

  // Compressible resources in J3D have a relocation table (necessary for
  // interop with other animations that access by index)
  reader.seekSet(g.start + ofsShapeLut);

  bool sorted = true;
  for (int i = 0; i < size; ++i) {
    ctx.shapeIdLut[i] = TRY(reader.U16());

    if (ctx.shapeIdLut[i] != i)
      sorted = false;
  }

  if (!sorted) {
    DebugReport("Shape IDS are remapped.\n");
  }

  // Unused
  // reader.seekSet(ofsStringTable + g.start);
  // const auto nameTable = readNameTable(reader);

  std::array<s16, 10> mtxListLast;
  for (int si = 0; si < size; ++si) {
    auto& shape = ctx.mdl.shapes[si];
    reader.seekSet(g.start + ofsShapeData + ctx.shapeIdLut[si] * 0x28);
    shape.id = ctx.shapeIdLut[si];
    // printf("Shape (index=%u, id=%u) {\n", si, shape.id);
    // shape.name = nameTable[si];
    shape.mode = TRY(reader.Enum8<ShapeData::Mode>());
    TRY(reader.U8());
    // Number of matrix primitives (mtxGrpCnt)
    auto num_matrix_prims = TRY(reader.U16());
    // printf("   num_matrix_prims=%u\n", (u32)num_matrix_prims);
    auto ofs_vcd_list = TRY(reader.U16());
    // printf("   ofs_vcd_list=%u\n", (u32)ofs_vcd_list);
    // current mtx/mtx list
    auto first_matrix_list = TRY(reader.U16());
    // printf("   first_matrix_list=%u\n", (u32)first_matrix_list);
    // "Packet" or mtx prim summary (accessor) idx
    auto first_mtx_prim_idx = TRY(reader.U16());
    // printf("   first_mtx_prim_idx=%u\n", (u32)first_mtx_prim_idx);
    EXPECT(first_matrix_list == first_mtx_prim_idx);
    TRY(reader.U16());
    shape.bsphere = TRY(reader.F32());
    shape.bbox.min = TRY(readVec3(reader));
    shape.bbox.max = TRY(readVec3(reader));
    // printf("}\n");

    // Read VCD attributes
    reader.seekSet(g.start + ofsVcdList + ofs_vcd_list);
    gx::VertexAttribute attr = gx::VertexAttribute::Undefined;
    while ((attr = static_cast<gx::VertexAttribute>(TRY(reader.U32()))) !=
           gx::VertexAttribute::Terminate) {
      shape.mVertexDescriptor.mAttributes[attr] =
          TRY(reader.Enum32<gx::VertexAttributeType>());
    }

    // Calculate the VCD bitfield
    shape.mVertexDescriptor.calcVertexDescriptorFromAttributeList();
    // const u32 vcd_size = (u32)shape.mVertexDescriptor.getVcdSize();

    // Read the two-layer primitives

    for (u16 i = 0; i < num_matrix_prims; ++i) {
      const u32 prim_idx = first_mtx_prim_idx + i;
      reader.seekSet(g.start + ofsMtxPrimAccessor + prim_idx * 8);
      const auto [dlSz, dlOfs] = reader.getUnsafe().readX<u32, 2>();

      struct MatrixData {
        s16 current_matrix;
        std::vector<s16> matrixList; // DRW1
      };
      auto readMatrixData = [&, ofsDrwIndices = ofsDrwIndices,
                             ofsMtxData = ofsMtxData]() -> Result<MatrixData> {
        // Assumption: Indexed by raw index *not* potentially remapped ID
        oishii::Jump<oishii::Whence::Set> j(reader.getUnsafe(),
                                            g.start + ofsMtxData +
                                                (first_matrix_list + i) * 8);
        MatrixData out;
        out.current_matrix = TRY(reader.S16());
        u16 list_size = TRY(reader.U16());
        s32 list_start = TRY(reader.S32());
        out.matrixList.resize(list_size);
        {
          oishii::Jump<oishii::Whence::Set> d(
              reader.getUnsafe(), g.start + ofsDrwIndices + list_start * 2);
          for (u16 i = 0; i < list_size; ++i) {
            out.matrixList[i] = TRY(reader.S16());
            if (out.matrixList[i] == -1)
              out.matrixList[i] = mtxListLast[i];
            else
              mtxListLast[i] = out.matrixList[i];
          }
        }
        return out;
      };

      // Mtx Prim Data
      MatrixData mtxPrimHdr = TRY(readMatrixData());
      gx::MatrixPrimitive& mprim = shape.mMatrixPrimitives.emplace_back(
          mtxPrimHdr.current_matrix, mtxPrimHdr.matrixList);

      struct SHP1_MPrim : librii::gpu::IMeshDLDelegate {
        librii::gx::IndexedPrimitive&
        addIndexedPrimitive(gx::PrimitiveType type, u16 nVerts) override {
          return mprim.mPrimitives.emplace_back(type, nVerts);
        }
        SHP1_MPrim(gx::MatrixPrimitive& mp) : mprim(mp) {}

      private:
        gx::MatrixPrimitive& mprim;
      } mprim_del(mprim);
      TRY(DecodeMeshDisplayList(reader.getUnsafe(), g.start + ofsDL + dlOfs,
                                dlSz, mprim_del, shape.mVertexDescriptor,
                                &ctx.mVertexBufferMaxIndices));
    }
  }

  return {};
}

template <typename T, u32 bodyAlign = 1, u32 entryAlign = 1,
          bool compress = true>
class CompressableVector : public oishii::Node {
  struct Child : public oishii::Node {
    Child(const CompressableVector& parent, u32 index)
        : mParent(parent), mIndex(index) {
      mId = std::to_string(index);
      getLinkingRestriction().alignment = entryAlign;
      // getLinkingRestriction().setFlag(oishii::LinkingRestriction::PadEnd);
      getLinkingRestriction().setLeaf();
    }
    Result write(oishii::Writer& writer) const noexcept override {
      mParent.getEntry(mIndex).write(writer);
      return {};
    }
    const CompressableVector& mParent;
    const u32 mIndex;
  };
  struct ContainerNode : public oishii::Node {
    ContainerNode(const CompressableVector& parent, const std::string& id)
        : mParent(parent) {
      mId = id;
      getLinkingRestriction().alignment = bodyAlign;
    }
    Result gatherChildren(NodeDelegate& d) const noexcept override {
      for (u32 i = 0; i < mParent.getNumEntries(); ++i)
        d.addNode(std::make_unique<Child>(mParent, i));
      return {};
    }
    const CompressableVector& mParent;
  };

public:
  std::unique_ptr<oishii::Node> spawnNode(const std::string& id) const {
    return std::make_unique<ContainerNode>(*this, id);
  }

  u32 append(const T& entry) {
    const auto found = std::find(mEntries.begin(), mEntries.end(), entry);
    if (found == mEntries.end() || !compress) {
      mEntries.push_back(entry);
      return mEntries.size() - 1;
    }
    return found - mEntries.begin();
  }
  int find(const T& entry) const {
    int outIdx = -1;
    for (int i = 0; i < mEntries.size(); ++i)
      if (entry == mEntries[i])
        outIdx = i;
    // Intentionally complete
    // TODO: rfind
    return outIdx;
  }
  u32 getNumEntries() const { return mEntries.size(); }
  const T& getEntry(u32 idx) const {
    assert(idx < mEntries.size());
    return mEntries[idx];
  }
  T& getEntry(u32 idx) {
    assert(idx < mEntries.size());
    return mEntries[idx];
  }

protected:
  std::vector<T> mEntries;
};
struct WriteableVertexDescriptor : librii::gx::VertexDescriptor {
  WriteableVertexDescriptor(const VertexDescriptor& d) {
    *(VertexDescriptor*)this = d;
  }
  void write(oishii::Writer& writer) const noexcept {
    for (auto& x : mAttributes) {
      writer.write<u32>(static_cast<u32>(x.first));
      writer.write<u32>(static_cast<u32>(x.second));
    }
    writer.write<u32>(static_cast<u32>(gx::VertexAttribute::Terminate));
    writer.write<u32>(0);
  }
};
struct WriteableMatrixList : public std::vector<s16> {
  WriteableMatrixList(const std::vector<s16>& parent) {
    *(std::vector<s16>*)this = parent;
  }
  void write(oishii::Writer& writer) const noexcept {
    for (int i = 0; i < size(); ++i)
      writer.write<u16>(at(i));

    //	while (writer.tell() % 32)
    //		writer.write<u8>(0);
  }
};

struct SHP1Node final : public oishii::Node {
  SHP1Node(const J3dModel& model) : mModel(model) {
    mId = "SHP1";
    mLinkingRestriction.alignment = 32;

    for (const auto& shp : mModel.shapes) {
      mVcdPool.append(shp.mVertexDescriptor);
      for (const auto& mp : shp.mMatrixPrimitives)
        mMtxListPool.append(mp.mDrawMatrixIndices);
    }
  }

  CompressableVector<WriteableVertexDescriptor, 32, 16> mVcdPool;
  CompressableVector<WriteableMatrixList,
#ifdef ALIGN_MTX_CHILDS
                     32, 32,
#else
                     0, 2,
#endif
                     false>
      mMtxListPool;
  Result write(oishii::Writer& writer) const noexcept override {
    // VCD List compression compute.
    //	struct VCDHasher
    //	{
    //		std::size_t operator()(const VertexDescriptor& v) const { return
    // v.mBitfield; }
    //	};
    //	std::map<VertexDescriptor, int, VCDHasher> vcd;
    //	for (auto& shp : mModel.mShapes)
    //		vcd[shp.mVertexDescriptor] = 0;

    writer.write<u32, oishii::EndianSelect::Big>('SHP1');
    writer.writeLink<s32>({*this}, {*this, oishii::Hook::EndOfChildren});

    writer.write<u16>(mModel.shapes.size());
    writer.write<u16>(-1);

    writer.writeLink<u32>({*this}, {"ShapeData"});
    writer.writeLink<u32>({*this}, {"LUT"});
    writer.write<u32>(0); // writer.writeLink<u32>({ *this }, { "NameTable" });
    writer.writeLink<u32>({*this}, {"VCDList"});
    writer.writeLink<u32>({*this}, {"MTXList"});
    writer.writeLink<u32>({*this}, {"DLData"});
    writer.writeLink<u32>({*this}, {"MTXData"});
    writer.writeLink<u32>({*this}, {"MTXGrpHdr"});
    // Shape Data
    // Remap table
    // Unused Name table
    // String
    // VCD List
    // DRW Table
    // DL data
    // MTX data
    // MTX Prim hdrs
    return {};
  }
  enum class SubNodeID {
    ShapeData,
    LUT,
    NameTable,
    VCDList,
    MTXList,
    DLData,
    MTXData,
    MTXGrpHdr,
    Max,

    _VCDChild = Max,
    _DLChild,
    _MTXChild,
    _MTXGrpChild,
    _DLChildMPrim,
    _MTXDataChild,
    _MTXListChild,
    _MTXListChildMPrim
  };
  struct SubNode : public oishii::Node {
    const SHP1Node& mParent;
    SubNode(const J3dModel& mdl, SubNodeID id, const SHP1Node& parent,
            int polyId = -1, int MPrimId = -1)
        : mParent(parent), mMdl(mdl), mSID(id), mPolyId(polyId),
          mMpId(MPrimId) {
      u32 align = 4;
      bool leaf = true;
      switch (id) {
      case SubNodeID::ShapeData:
        mId = "ShapeData";
        align = 4;
        break;
      case SubNodeID::LUT:
        mId = "LUT";
        align = 4;
        getLinkingRestriction().PadEnd = true;
        break;
      case SubNodeID::NameTable:
        mId = "NameTable";
        break;
      case SubNodeID::DLData:
        mId = "DLData";
        align = 32;
        leaf = false;
        break;
      case SubNodeID::MTXData:
        mId = "MTXData";
        align = 4;
        leaf = false;
        break;

      case SubNodeID::_MTXDataChild:
        mId = std::to_string(mPolyId);
        align = 4;
        leaf = true;
        break;
      case SubNodeID::MTXGrpHdr:
        mId = "MTXGrpHdr";
        align = 2;
        leaf = false;
        break;
      case SubNodeID::_DLChild:
        mId = std::to_string(mPolyId);
        align = 32;
        leaf = false;
        break;
      case SubNodeID::_MTXChild:
        mId = std::to_string(mPolyId);
        align = 4;
        leaf = true;
        break;
      case SubNodeID::_MTXGrpChild:
        mId = std::to_string(mPolyId);
        align = 4;
        leaf = true;
        break;
      case SubNodeID::_DLChildMPrim:
        mId = std::to_string(MPrimId);
        align = 32;
        leaf = true;
        break;
      default:
        break;
      }
      if (leaf)
        getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = align;
    }

    std::expected<void, std::string>
    write2(oishii::Writer& writer) const noexcept {
      switch (mSID) {
      case SubNodeID::ShapeData: {
        for (int i = 0; i < mMdl.shapes.size(); ++i) {
          const auto& shp = mMdl.shapes[i];

          writer.write<u8>(static_cast<u8>(shp.mode));
          writer.write<u8>(0xff);
          writer.write<u16>(shp.mMatrixPrimitives.size());
          int vcd = mParent.mVcdPool.find(shp.mVertexDescriptor);
          writer.writeLink<u16>({"SHP1::VCDList"},
                                {std::string("SHP1::VCDList::") +
                                 std::to_string(vcd)}); // offset into VCD list
          // TODO -- we don't support compression..
          int mpi = 0;
          for (int j = 0; j < i; ++j) {
            mpi += mMdl.shapes[j].mMatrixPrimitives.size();
          }
          writer.write<u16>(mpi); // Matrix list index of this prim
          writer.write<u16>(mpi); // Matrix primitive index
          writer.write<u16>(0xffff);
          writer.write<f32>(shp.bsphere);
          shp.bbox.min >> writer;
          shp.bbox.max >> writer;
        }
        writer.alignTo(4);

        break;
      }
      case SubNodeID::LUT:
        // TODO...
        for (int i = 0; i < mMdl.shapes.size(); ++i)
          writer.write<u16>(i);
        break;
      case SubNodeID::NameTable:
        break; // Always zero
      case SubNodeID::VCDList:
        break; // Children do the writing.
      case SubNodeID::_VCDChild:
        // TODO
        break;
      case SubNodeID::MTXList:
        break; // Children write

      case SubNodeID::_MTXListChild: {
        for (auto x : mtxListPool->at(mPolyId))
          // TODO: Move -1 to here..
          writer.write<u16>(x);
        break;
      }
      case SubNodeID::DLData:
        break; // Children write
      case SubNodeID::_DLChild:
        break; // MPrims write..
      case SubNodeID::_DLChildMPrim: {
        const auto& poly = mMdl.shapes[mPolyId];
        for (auto& prim : poly.mMatrixPrimitives[mMpId].mPrimitives) {
          writer.write<u8>(gx::EncodeDrawPrimitiveCommand(prim.mType));
          writer.write<u16>(prim.mVertices.size());
          for (const auto& v : prim.mVertices) {
            for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a) {
              if (poly.mVertexDescriptor[(gx::VertexAttribute)a]) {
                switch (poly.mVertexDescriptor.mAttributes.at(
                    (gx::VertexAttribute)a)) {
                case gx::VertexAttributeType::None:
                  break;
                case gx::VertexAttributeType::Byte:
                  writer.write<u8>(v[(gx::VertexAttribute)a]);
                  break;
                case gx::VertexAttributeType::Short:
                  writer.write<u16>(v[(gx::VertexAttribute)a]);
                  break;
                case gx::VertexAttributeType::Direct:
                  if (((gx::VertexAttribute)a) !=
                      gx::VertexAttribute::PositionNormalMatrixIndex) {
                    EXPECT(!"Direct vertex data is unsupported.");
                  }
                  writer.write<u8>(v[(gx::VertexAttribute)a]);
                  break;
                default:
                  EXPECT("!Unknown vertex attribute format.");
                }
              }
            }
          }
        }
        // DL pad
        while (writer.tell() % 32)
          writer.write<u8>(0);
        break;
      }
      case SubNodeID::MTXData:
        break; // Children write
      case SubNodeID::_MTXDataChild: {

        u32 num = 0;
        for (int j = 0; j < mPolyId; ++j)
          for (auto& mp : mMdl.shapes[j].mMatrixPrimitives)
            num += mp.mDrawMatrixIndices.size();
        for (const auto& x : mMdl.shapes[mPolyId].mMatrixPrimitives) {
          writer.write<u16>(x.mCurrentMatrix);
          // listSize, listStartIndex
          writer.write<u16>(x.mDrawMatrixIndices.size());

          {
            //	u32 mpId = mParent.mMtxListPool.find({ x.mDrawMatrixIndices });
            //	writer.writeLink<u32>({
            //		{"SHP1::MTXList"},
            //		{std::string("SHP1::MTXList::") + std::to_string(mpId)
            //}, 		2 });
            // Never compressed...
            writer.write<u32>(num);
            num += x.mDrawMatrixIndices.size();
          }
        }
        break;
      }
      case SubNodeID::MTXGrpHdr:
        break; // Children write
      case SubNodeID::_MTXGrpChild:
        for (int i = 0; i < mMdl.shapes[mPolyId].mMatrixPrimitives.size();
             ++i) {
          std::string front =
              std::string("SHP1::DLData::") + std::to_string(mPolyId) + "::";
          // DL size
          writer.writeLink<u32>(
              {front + std::to_string(i)},
              {front + std::to_string(i), oishii::Hook::RelativePosition::End});
          // Relative DL offset
          writer.writeLink<u32>({"SHP1::DLData"}, {front + std::to_string(i)});
        }
        break;
      default:
        break;
      }
      return {};
    }

    Result gatherChildren(NodeDelegate& d) const noexcept override {
      switch (mSID) {
      case SubNodeID::ShapeData:
      case SubNodeID::LUT:
      case SubNodeID::NameTable:
      case SubNodeID::_VCDChild:
      case SubNodeID::_MTXChild:
      case SubNodeID::_MTXGrpChild:
      case SubNodeID::_MTXDataChild:
        break;
      case SubNodeID::DLData:
        for (int i = 0; i < mMdl.shapes.size(); ++i)
          d.addNode(
              std::make_unique<SubNode>(mMdl, SubNodeID::_DLChild, mParent, i));
        break;
      case SubNodeID::MTXData:
        for (int i = 0; i < mMdl.shapes.size(); ++i)
          d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_MTXDataChild,
                                              mParent, i));
        break;
      case SubNodeID::MTXGrpHdr:
        for (int i = 0; i < mMdl.shapes.size(); ++i)
          d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_MTXGrpChild,
                                              mParent, i));
        break;
      case SubNodeID::_DLChild:
        for (int i = 0; i < mMdl.shapes[mPolyId].mMatrixPrimitives.size(); ++i)
          d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_DLChildMPrim,
                                              mParent, mPolyId, i));
        break;
      default:
        break;
      }

      return {};
    }

    const J3dModel& mMdl;
    const SubNodeID mSID;
    int mPolyId = -1;
    int mMpId = -1;

    const std::vector<librii::gx::VertexDescriptor>* vcdPool;
    const std::vector<std::vector<s16>>* mtxListPool;
  };

  Result gatherChildren(NodeDelegate& d) const noexcept override {
    auto addSubNode = [&](SubNodeID ID) {
      d.addNode(std::make_unique<SubNode>(mModel, ID, *this, -1, -1));
    };

    addSubNode(SubNodeID::ShapeData);
    addSubNode(SubNodeID::LUT);
    d.addNode(mVcdPool.spawnNode("VCDList"));
    d.addNode(mMtxListPool.spawnNode("MTXList"));
    addSubNode(SubNodeID::DLData);
    addSubNode(SubNodeID::MTXData);
    addSubNode(SubNodeID::MTXGrpHdr);
    return {};
  }
  const J3dModel& mModel;
};

std::unique_ptr<oishii::Node> makeSHP1Node(BMDExportContext& ctx) {
  return std::make_unique<SHP1Node>(ctx.mdl);
}

} // namespace librii::j3d
