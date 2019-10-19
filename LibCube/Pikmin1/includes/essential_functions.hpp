#pragma once

#define OISHII_ALIGNMENT_CHECK 0

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vector>
#include <ThirdParty/glm/glm.hpp>

namespace libcube {

// This can be used in other filetypes, its not just useful for Pikmin 1
static inline void skipChunk(oishii::BinaryReader& bReader, u32 offset)
{
	bReader.seek<oishii::Whence::Current>(offset);
}

namespace pikmin1 {

static inline void skipPadding(oishii::BinaryReader& bReader, u32 alignTo = 0x20)
{
	const u32 currentPos = bReader.tell();
	const u32 toRead = (~(alignTo - 1) & (currentPos + alignTo - 1)) - currentPos;
	skipChunk(bReader, toRead);
}



// abstraction of dispatch
template<typename T>
inline void readChunk(oishii::BinaryReader& reader, T& out)
{
	reader.dispatch<T, oishii::Direct, false>(out);
}

}

}
