#pragma once

namespace libcube {

template<typename T>
struct TextureDimensions
{
	T width, height;

	constexpr bool isPowerOfTwo() { return !(width & (width - 1)) && (!height & (height - 1)); }
	constexpr bool isBlockAligned() { return !(width % 0x20) && !(height % 0x20); }
};

} // namespace libcube
