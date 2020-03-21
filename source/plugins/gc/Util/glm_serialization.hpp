#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <oishii/v2/writer/binary_writer.hxx>
#include <vendor/glm/vec3.hpp>
#include <vendor/glm/vec2.hpp>

namespace libcube {

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

inline void operator>>(const glm::vec3& vec, oishii::v2::Writer& writer)
{
	writer.write(vec.x);
	writer.write(vec.y);
	writer.write(vec.z);
}
inline void operator>>(const glm::vec2& vec, oishii::v2::Writer& writer)
{
	writer.write(vec.x);
	writer.write(vec.y);
}

} // namespace libcube
