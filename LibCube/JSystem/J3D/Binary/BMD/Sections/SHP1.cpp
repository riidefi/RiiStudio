#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>

namespace libcube::jsystem {

void readSHP1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	
    if (!enterSection(ctx, 'SHP1'))
        return;

    ScopedSection g(reader, "Shapes");

    u16 size = reader.read<u16>();
    ctx.mdl.mShapes.resize(size);
    ctx.shapeIdLut.resize(size);
    reader.read<u16>();

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
        // u16 matrix_list_size; // Vector of matrix indices; limited by hardware
        // int matrix_list_start;
        // }
        // Each sampled by matrix primitives
        ofsMtxData, // "mtx data"

        // size + offset?
        // matrix primitive splits
        ofsMtxPrimAccessor // "mtx grp"
    ] = reader.readX<s32, 8>();
    reader.seekSet(g.start);

    // Compressible resources in J3D have a relocation table (necessary for interop with other animations that access by index)
    reader.seekSet(g.start + ofsShapeLut);

    bool sorted = true;
    for (int i = 0; i < size; ++i)
    {
        ctx.shapeIdLut[i] = reader.read<u16>();

        if (ctx.shapeIdLut[i] != i)
            sorted = false;
    }

    if (!sorted)
        DebugReport("Shape IDS are remapped.\n");


    // Unused
    // reader.seekSet(ofsStringTable + g.start);
    // const auto nameTable = readNameTable(reader);

    for (int si = 0; si < size; ++si)
    {
        auto& shape = ctx.mdl.mShapes[si];
        reader.seekSet(g.start + ofsShapeData + ctx.shapeIdLut[si] * 0x28);
        shape.id = ctx.shapeIdLut[si];
        // shape.name = nameTable[si];
        shape.mode = static_cast<ShapeData::Mode>(reader.read<u8>());
        assert(shape.mode < ShapeData::Mode::Max);
        reader.read<u8>();
        // Number of matrix primitives (mtxGrpCnt)
        auto num_matrix_prims = reader.read<u16>();
        auto ofs_vcd_list = reader.read<u16>();
        // current mtx/mtx list
        auto first_matrix_list = reader.read<u16>();
        // "Packet" or mtx prim summary (accessor) idx
        auto first_mtx_prim_idx = reader.read<u16>();
        reader.read<u16>();
        shape.bsphere = reader.read<f32>();
        shape.bbox << reader;

        // Read VCD attributes
        reader.seekSet(g.start + ofsVcdList + ofs_vcd_list);
        gx::VertexAttribute attr = gx::VertexAttribute::Undefined;
        while ((attr = reader.read<gx::VertexAttribute>()) != gx::VertexAttribute::Terminate)
            shape.mVertexDescriptor.mAttributes[attr] = reader.read<gx::VertexAttributeType>();

        // Calculate the VCD bitfield
        shape.mVertexDescriptor.calcVertexDescriptorFromAttributeList();
        const u32 vcd_size = shape.mVertexDescriptor.getVcdSize();

        // Read the two-layer primitives

        
        for (u16 i = 0; i < num_matrix_prims; ++i)
        {
            const u32 prim_idx = first_mtx_prim_idx + i;
            reader.seekSet(g.start + ofsMtxPrimAccessor + prim_idx * 8);
            const auto [dlSz, dlOfs] = reader.readX<u32, 2>();

            struct MatrixData
            {
                s16 current_matrix;
                std::vector<s16> matrixList; // DRW1
            };
            auto readMatrixData = [&, ofsDrwIndices=ofsDrwIndices, ofsMtxData=ofsMtxData]()
            {
                oishii::Jump<oishii::Whence::At> j(reader, g.start + ofsMtxData + first_matrix_list * 8);
                MatrixData out;
                out.current_matrix = reader.read<s16>();
                u16 list_size = reader.read<u16>();
                s32 list_start = reader.read<s32>();
                out.matrixList.resize(list_size);
                {
                    oishii::Jump<oishii::Whence::At> d(reader, g.start + ofsDrwIndices + list_start * 2);
                    for (u16 i = 0; i < list_size; ++i)
                    {
                        out.matrixList[i] = reader.read<s16>();
                    }
                }
                return out;
            };

            // Mtx Prim Data
            MatrixData mtxPrimHdr = readMatrixData();
            MatrixPrimitive& mprim = shape.mMatrixPrimitives.emplace_back(mtxPrimHdr.current_matrix, mtxPrimHdr.matrixList);
            
            // Now read prim data..
            // (Stripped down display list interpreter function)
            
            reader.seekSet(g.start + ofsDL + dlOfs);
            while (reader.tell() < g.start + ofsDL + dlOfs + dlSz)
            {
                const u8 tag = reader.read<u8, oishii::EndianSelect::Current, true>();
                if (tag == 0) break;
                assert(tag & 0x80 && "Unexpected GPU command in display list.");
                const gx::PrimitiveType type = gx::DecodeDrawPrimitiveCommand(tag);
                u16 nVerts = reader.read<u16, oishii::EndianSelect::Current, true>();
                IndexedPrimitive& prim = mprim.mPrimitives.emplace_back(type, nVerts);

                for (u16 vi = 0; vi < nVerts; ++vi)
                {
                    for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a)
                    {
                        if (shape.mVertexDescriptor[(gx::VertexAttribute)a])
                        {
                            u16 val = 0;

                            switch (shape.mVertexDescriptor.mAttributes[(gx::VertexAttribute)a])
                            {
                            case gx::VertexAttributeType::None:
                                break;
                            case gx::VertexAttributeType::Byte:
                                val = reader.read<u8, oishii::EndianSelect::Current, true>();
                                break;
                            case gx::VertexAttributeType::Short:
                                val = reader.read<u16, oishii::EndianSelect::Current, true>();
                                break;
                            case gx::VertexAttributeType::Direct:
                                if (((gx::VertexAttribute)a) != gx::VertexAttribute::PositionNormalMatrixIndex)
                                {
                                    assert(!"Direct vertex data is unsupported.");
                                    throw "";
                                }
                                val = reader.read<u8, oishii::EndianSelect::Current, true>(); // As PNM indices are always direct, we still use them in an all-indexed vertex
                                break;
                            default:
                                assert("!Unknown vertex attribute format.");
                                throw "";
                            }

                            prim.mVertices[vi][(gx::VertexAttribute)a] = val;
                        }
                    }
                }
            }
        }
    }
}

}
