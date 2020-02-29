#pragma once

namespace libcube {

enum class STBImage
{
	PNG,
	BMP,
	TGA,
	JPG,
	HDR,

	MAX
};

// 8 bit data -- For HDR pass f32s
void writeImageStb(const char* filename, STBImage type, int x, int y, int channel_component_count, const void* data);

inline void writeImageStbRGBA(const char* filename, STBImage type, int x, int y, const void* data)
{
	writeImageStb(filename, type, x, y, 4, data);
}
inline void writeImageStbRGB(const char* filename, STBImage type, int x, int y, const void* data)
{
	writeImageStb(filename, type, x, y, 3, data);
}

} // namespace libcube
