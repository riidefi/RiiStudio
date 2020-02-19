/*!
 * @file
 * @brief FIXME: Document
 */

#define _USE_MATH_DEFINES
#include <cmath>

#include "BMD.hpp"
#include <LibJ/J3D/Model.hpp>
#include <map>
#include <type_traits>
#include <algorithm>
#include <tuple>

#include <oishii/v2/writer/node.hxx>
#include <oishii/v2/writer/linker.hxx>
#include <oishii/v2/writer/hook.hxx>

#include "SceneGraph.hpp"
#include "Sections.hpp"

namespace libcube::jsystem {

void BmdIo::lex(BMDOutputContext& ctx, u32 sec_count) noexcept
{
	ctx.mSections.clear();
	for (u32 i = 0; i < sec_count; ++i)
	{
		const u32 secType = ctx.reader.read<u32>();
		const u32 secSize = ctx.reader.read<u32>();

		{
			oishii::JumpOut g(ctx.reader, secSize - 8);
			switch (secType)
			{
			case 'INF1':
			case 'VTX1':
			case 'EVP1':
			case 'DRW1':
			case 'JNT1':
			case 'SHP1':
			case 'MAT3':
			case 'MDL3':
			case 'TEX1':
				ctx.mSections[secType] = { ctx.reader.tell(), secSize };
				break;
			default:
				ctx.reader.warnAt("Unexpected section type.", ctx.reader.tell() - 8, ctx.reader.tell() - 4);
				break;
			}
		}
	}
}
void BmdIo::readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx)
{
	reader.setEndian(true);
	reader.expectMagic<'J3D2'>();


	u32 bmdVer = reader.read<u32>();
	if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
	{
		reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
		// error = true;
		return;
	}

	// TODO: Validate file size.
	// const auto fileSize =
		reader.read<u32>();
	const auto sec_count = reader.read<u32>();

	// Skip SVR3 header
	reader.seek<oishii::Whence::Current>(16);

	// Skim through sections
	lex(ctx, sec_count);

	// Read vertex data
	readVTX1(ctx);

	// Read joints
	readJNT1(ctx);

	// Read envelopes and draw matrices
	readEVP1DRW1(ctx);

	// Read shapes
	readSHP1(ctx);

	for (const auto& e : ctx.mVertexBufferMaxIndices)
	{
		switch (e.first)
		{
		case gx::VertexBufferAttribute::Position:
			if (ctx.mdl.mBufs.pos.mData.size() != e.second + 1)
			{
				printf("The position vertex buffer currently has %u greedily-claimed entries due to 32B padding; %u are used.\n", (u32)ctx.mdl.mBufs.pos.mData.size(), e.second + 1);
				ctx.mdl.mBufs.pos.mData.resize(e.second + 1);
			}
			break;
		case gx::VertexBufferAttribute::Color0:
		case gx::VertexBufferAttribute::Color1:
		{
			auto& buf = ctx.mdl.mBufs.color[(int)e.first - (int)gx::VertexBufferAttribute::Color0];
			if (buf.mData.size() != e.second + 1)
			{
				printf("The color buffer currently has %u greedily-claimed entries due to 32B padding; %u are used.\n", (u32)buf.mData.size(), e.second + 1);
				buf.mData.resize(e.second + 1);
			}
			break;
		}
		case gx::VertexBufferAttribute::Normal:
		{
			auto& buf = ctx.mdl.mBufs.norm;
			if (buf.mData.size() != e.second + 1)
			{
				printf("The normal buffer currently has %u greedily-claimed entries due to 32B padding; %u are used.\n", (u32)buf.mData.size(), e.second + 1);
				buf.mData.resize(e.second + 1);
			}
			break;
		}
		case gx::VertexBufferAttribute::TexCoord0:
		case gx::VertexBufferAttribute::TexCoord1:
		case gx::VertexBufferAttribute::TexCoord2:
		case gx::VertexBufferAttribute::TexCoord3:
		case gx::VertexBufferAttribute::TexCoord4:
		case gx::VertexBufferAttribute::TexCoord5:
		case gx::VertexBufferAttribute::TexCoord6:
		case gx::VertexBufferAttribute::TexCoord7:
		{
			auto& buf = ctx.mdl.mBufs.uv[(int)e.first - (int)gx::VertexBufferAttribute::TexCoord0];
			if (buf.mData.size() != e.second + 1)
			{
				printf("The UV buffer currently has %u greedily-claimed entries due to 32B padding; %u are used.\n", (u32)buf.mData.size(), e.second + 1);
				buf.mData.resize(e.second + 1);
			}
		}
		default:
			break; // TODO
		}
	}



	// Read materials
	readMAT3(ctx);

	// Read TEX1
	readTEX1(ctx);


	// Read INF1
	readINF1(ctx);

	// Read MDL3
}

#ifndef NDEBUG
std::vector<u8> __lastReadDataForWriteDebug;
#endif

void BmdIo::read(px::Dynamic& state, oishii::BinaryReader& reader)
{
#ifndef NDEBUG
	__lastReadDataForWriteDebug.resize(reader.endpos());
	memcpy(__lastReadDataForWriteDebug.data(), reader.getStreamStart(), __lastReadDataForWriteDebug.size());
#endif

	// reader.add_bp(0x37ba0, 4);
	// oishii::BinaryReader::ScopedRegion g(reader, "J3D Binary Model Data (.bmd)");

	J3DCollection* pCol = dynamic_cast<J3DCollection*>(state.mOwner.get());
	assert(pCol);
	if (!pCol) return;
	pCol->getFolder<J3DModel>()->push(std::make_unique<J3DModel>());
	BMDOutputContext ctx{ pCol->getModels()[0], *pCol, reader };

	// reader.add_bp<u32>(8);
	// reader.add_bp(0x6970, 16);
	// reader.add_bp(0x1d480, 16);

	readBMD(reader, ctx);
}


struct BMDFile : public oishii::v2::Node
{
	static const char* getNameId() { return "JSystem Binary Model Data"; }

	Result write(oishii::v2::Writer& writer) const noexcept
	{
		// Hack
		writer.write<u32, oishii::EndianSelect::Big>('J3D2');
		writer.write<u32, oishii::EndianSelect::Big>(bBDL ? 'bdl4' : 'bmd3');

		// Filesize
		writer.writeLink<s32>(oishii::v2::Link{
			oishii::v2::Hook(*this),
			oishii::v2::Hook(*this, oishii::v2::Hook::EndOfChildren) });

		// 8 sections
		writer.write<u32>(bBDL ? 9 : 8);

		// SubVeRsion
		writer.write<u32, oishii::EndianSelect::Big>('SVR3');
		for (int i = 0; i < 3; ++i)
			writer.write<u32>(-1);

		return {};
	}
	Result gatherChildren(oishii::v2::Node::NodeDelegate& ctx) const
	{
		BMDExportContext exp{ mCollection->getModels()[0], *mCollection };

		auto addNode = [&](std::unique_ptr<oishii::v2::Node> node)
		{
			node->getLinkingRestriction().alignment = 32;
			ctx.addNode(std::move(node));
		};

		addNode(makeINF1Node(exp));
		addNode(makeVTX1Node(exp));
		addNode(makeEVP1Node(exp));
		addNode(makeDRW1Node(exp));
		addNode(makeJNT1Node(exp));
		addNode(makeSHP1Node(exp));
		addNode(makeMAT3Node(exp));
		addNode(makeTEX1Node(exp));
		return {};
	}

	J3DCollection* mCollection;
	bool bBDL = true;
	bool bMimic = true;
};
void BMD_Pad(char* dst, u32 len)
{
	//assert(len < 30);
	memcpy(dst, "This is padding data to alignment.....", len);
}
void exportBMD(oishii::v2::Writer& writer, J3DCollection& collection)
{
	// writer.attachDataForMatchingOutput(__lastReadDataForWriteDebug);
	writer.add_bp(0x37ba0, 16);

	oishii::v2::Linker linker;

	auto bmd = std::make_unique<BMDFile>();
	bmd->bBDL = collection.bdl;
	bmd->bMimic = true;
	bmd->mCollection = &collection;

	linker.mUserPad = &BMD_Pad;
	writer.mUserPad = &BMD_Pad;

	linker.gather(std::move(bmd), "");
	linker.write(writer);
}

void BmdIo::write(px::Dynamic& state, oishii::v2::Writer& writer)
{
	J3DCollection* pCol = reinterpret_cast<J3DCollection*>(state.mBase);
	assert(pCol);
	if (!pCol) return;

	exportBMD(writer, *pCol);
}

} // namespace libcube::jsystem
