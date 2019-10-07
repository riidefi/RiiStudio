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
