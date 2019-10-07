/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include "BMD.hpp"
#include "../Model.hpp"

namespace libcube { namespace jsystem {

struct J3DModelLoadingContext
{
	J3DModel& mdl;
	bool error = false;
};

struct INF1Handler
{
	struct SceneGraph
	{
		static constexpr const char name[] = "Scenegraph";

		enum class ByteCodeOp : u16
		{
			End,
			Open,
			Close,

			Joint = 0x10,
			Material,
			Shape,

			Unitialized = -1
		};

		static_assert(sizeof(ByteCodeOp) == 2, "Invalid enum size.");

		struct ByteCodeCmd
		{
			ByteCodeOp op;
			s16 idx;

			ByteCodeCmd(oishii::BinaryReader& reader)
			{
				transfer(reader);
			}
			ByteCodeCmd()
				: op(ByteCodeOp::Unitialized), idx(-1)
			{}

			template<typename T>
			void transfer(T& stream)
			{
				stream.transfer<ByteCodeOp>(op);
				stream.transfer<s16>(idx);
			}
			
		};

		static void onRead(oishii::BinaryReader& reader, J3DModelLoadingContext& ctx)
		{
			for (ByteCodeCmd cmd(reader); cmd.op != ByteCodeOp::End; cmd.transfer(reader))
			{
				// TODO
			}
		}
	};


	static constexpr const char name[] = "Hierarchy/Display Information (INF1)";

	static void onRead(oishii::BinaryReader& reader, J3DModelLoadingContext& ctx)
	{
		u32 flag = reader.read<u32>();

		// TODO -- Use these for validation
		u32 nPacket = reader.read<u32>();
		u32 nVertex = reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);

		reader.dispatch<SceneGraph, oishii::Indirection<-8, s32>>(ctx);
	}
};


struct BMDHandler
{
	static constexpr const char name[] = "JSystem J3D Binary Model Data (.bmd)";

	static void onRead(oishii::BinaryReader& reader, J3DModelLoadingContext& ctx)
	{
		reader.expectMagic<'J3D2'>();
		u32 bmdVer = reader.read<u32>();
		if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
		{
			reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
			ctx.error = true;
			return;
		}

		const u32 fileSize = reader.read<u32>();
		const u32 sec_count = reader.read<u32>();
		// Skip SVR3 section
		reader.seek<oishii::Whence::Current>(16);


		for (u32 i = 0; i < sec_count; ++i)
		{
			const u32 secType = reader.read<u32>();
			const u32 secSize = reader.read<u32>();

			oishii::JumpOut g(reader, secSize - 8);
			{
				switch (secType)
				{
				case 'INF1':
					reader.dispatch<INF1Handler, oishii::Direct, false>(ctx);
					break;

				default:
					reader.warnAt("Unexpected section type.", reader.tell() - 8, reader.tell() - 4);
					break;
				}
			}
		}

	}
};


bool BMDImporter::tryRead(oishii::BinaryReader& reader, pl::FileState& state)
{
	J3DCollection& collection = static_cast<J3DCollection&>(state);
	collection.mModels.emplace_back();

	J3DModelLoadingContext BMDLoad = {
		*collection.mModels.back().get(),
		false
	};

	reader.dispatch<BMDHandler, oishii::Direct, false>(BMDLoad);
	
	return !BMDLoad.error;
}

} } // namespace libcube::jsystem
