#pragma once

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

// Pikmin 1 pads to the nearest multiple of 32 bytes (0x20)
static inline void skipPadding(oishii::BinaryReader& bReader)
{
	const u32 currentPos = bReader.tell();
	const u32 toRead = (~(0x20 - 1) & (currentPos + 0x20 - 1)) - currentPos;
	skipChunk(bReader, toRead);
}

// Vector reading functions
template<typename TComponent, int TComponentCount>
inline void read(oishii::BinaryReader& reader, glm::vec<TComponentCount, TComponent, glm::defaultp>& out);

template<>
inline void read<f32, 3>(oishii::BinaryReader& reader, glm::vec3& out)
{
	out.x = reader.read<f32>();
	out.y = reader.read<f32>();
	out.z = reader.read<f32>();
}

template<>
inline void read<f32, 2>(oishii::BinaryReader& reader, glm::vec2& out)
{
	out.x = reader.read<f32>();
	out.y = reader.read<f32>();
}
template<typename TComponent, int TComponentCount>
inline void operator<<(glm::vec<TComponentCount, TComponent, glm::defaultp>& out, oishii::BinaryReader& reader);

template<>
inline void operator<< <f32, 3>(glm::vec3& out, oishii::BinaryReader& reader)
{
	out.x = reader.read<f32>();
	out.y = reader.read<f32>();
	out.z = reader.read<f32>();
}
template<>
inline void operator<< <f32, 2>(glm::vec2& out, oishii::BinaryReader& reader)
{
	out.x = reader.read<f32>();
	out.y = reader.read<f32>();
}

// abstraction of dispatch
template<typename T>
inline void readChunk(oishii::BinaryReader& reader, T& out)
{
	reader.dispatch<T, oishii::Direct, false>(out);
}

}

}
