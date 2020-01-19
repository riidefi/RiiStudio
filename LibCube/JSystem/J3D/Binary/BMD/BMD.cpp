/*!
 * @file
 * @brief FIXME: Document
 */

#define _USE_MATH_DEFINES
#include <cmath>

#include "BMD.hpp"
#include <LibCube/JSystem/J3D/Model.hpp>
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

void BMDImporter::lex(BMDOutputContext& ctx, u32 sec_count) noexcept
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
void BMDImporter::readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx)
{
	reader.setEndian(true);
	reader.expectMagic<'J3D2'>();


	u32 bmdVer = reader.read<u32>();
	if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
	{
		reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
		error = true;
		return;
	}

	// TODO: Validate file size.
	const auto fileSize = reader.read<u32>();
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

	// Read TEX1

	// Read materials
	readMAT3(ctx);

	// Read INF1
	readINF1(ctx);

	// Read MDL3
}
bool BMDImporter::tryRead(oishii::BinaryReader& reader, pl::FileState& state)
{
	//oishii::BinaryReader::ScopedRegion g(reader, "J3D Binary Model Data (.bmd)");
	auto& j3dc = static_cast<J3DCollection&>(state);
	BMDOutputContext ctx{ j3dc.mModel, reader };

	// reader.add_bp<u32>(8);
	//reader.add_bp(0x2cc0, 16);

	readBMD(reader, ctx);
	return !error;
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
		BMDExportContext exp{ mCollection->mModel };

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
		//	addNode(makeSHP1Node(exp));
		//	addNode(makeMAT3Node(exp));
		//	addNode(makeTEX1Node(exp));
		return {};
	}

	J3DCollection* mCollection;
	bool bBDL = true;
	bool bMimic = true;
};
void BMD_Pad(char* dst, u32 len)
{
	assert(len < 30);
	memcpy(dst, "This is padding data to align.....", len);
}
void exportBMD(oishii::v2::Writer& writer, J3DCollection& collection)
{
	oishii::v2::Linker linker;

	auto bmd = std::make_unique<BMDFile>();
	bmd->bBDL = collection.bdl;
	bmd->bMimic = true;
	bmd->mCollection = &collection;

	linker.mUserPad = &BMD_Pad;

	linker.gather(std::move(bmd), "");
	linker.write(writer);
}

bool BMDExporter::write(oishii::v2::Writer& writer, pl::FileState& state)
{
	exportBMD(writer, static_cast<J3DCollection&>(state));

	return true;
}

} // namespace libcube::jsystem
