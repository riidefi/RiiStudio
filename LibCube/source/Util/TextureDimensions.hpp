#pragma once

namespace libcube {

template<typename T>
struct TextureDimensions
{
	T width, height;

	// TODO
	constexpr bool isPowerOfTwo() { return false;}
	constexpr bool isBlockAligned() { return false;}
};

} // namespace libcube
