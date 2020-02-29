#pragma once

// This is based on "Metaphrasis" by Armin Tamzarian (https://github.com/ArminTamzarian/metaphrasis/blob/master/Metaphrasis.cpp)
// It has been updated for modern C++ on PCs.

#include <stdint.h>
#include <malloc.h>
#include <string.h>

#include <memory>

#define RGBA_TO_IA4(x) ((x & 0x0000f000) >> 8) | ((x & 0x000000f0) >> 4)
#define RGBA_TO_IA8(x) x & 0x0000ffff
#define RGBA_TO_RGB565(x) ((x & 0xf8000000) >> 16) | ((x & 0x00fc0000) >> 13) | ((x & 0x0000f800) >> 11)
#define RGBA_TO_RGB555(x) ((x & 0xf8000000) >> 17) | ((x & 0x00f80000) >> 14) | ((x & 0x0000f800) >> 11) | 0x8000
#define RGBA_TO_RGB444(x) ((x & 0xf0000000) >> 17) | ((x & 0x00f00000) >> 13) | ((x & 0x0000f000) >> 9) | ((x & 0x000000e0) >> 5)
#define RGBA_TO_RGB5A3(x) (x & 0xff) < 0xe0 ? RGBA_TO_RGB444(x) : RGBA_TO_RGB555(x)

std::unique_ptr<uint32_t[]> convertBufferToI4(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);
std::unique_ptr<uint32_t[]> convertBufferToI8(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);
std::unique_ptr<uint32_t[]> convertBufferToIA4(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);
std::unique_ptr<uint32_t[]> convertBufferToIA8(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);
std::unique_ptr<uint32_t[]> convertBufferToRGBA8(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);
std::unique_ptr<uint32_t[]> convertBufferToRGB565(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);
std::unique_ptr<uint32_t[]> convertBufferToRGB5A3(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight);


/**
 * Downsample the specified RGBA value data buffer to an IA4 value.
 *
 * This routine downsamples the given RGBA data value into the IA4 texture data format.
 *
 * <strong>Format Explanation</strong>
 * \n
 * IA4 is a greyscale color format with a 4 bit Alpha component. In order to convert from the given RGBA color format simply concatenate
 * the first 4 MSB from any color component (in this case the Blue component to decrease the number of place shifts performed) to
 * the first 4 MSB from the Alpha component.
 * \n\n
 * <code>
 * RGBA (32bit):  r7r6r5r4r3r2r1r0|g7g6g5g4g3g2g1g0|b7b6b5b4b3b2b1b|0a7a6a5a4a3a2a1a0
 * \n
 * RGBIA4 (8bit): b7b6b5b4a7a6a5a4
 * </code>
 *
 * @param rgba	A 32-bit RGBA value to convert to the IA4 format.
 * @return The IA4 value of the given RGBA value.
 */

constexpr uint8_t convertRGBAToIA4(uint32_t rgba) {
	return RGBA_TO_IA4(rgba);
}

/**
 * Downsample the specified RGBA value data buffer to an IA8 value.
 *
 * This routine downsamples the given RGBA data value into the IA8 texture data format.
 *
 * <strong>Format Explanation</strong>
 * \n
 * IA8 is a greyscale color format with a full 8 bit Alpha component. In order to convert from the given RGBA color format simply concatenate
 * the entire 8 bit information from any color component (in this case the Blue component to decrease the number of place shifts performed) to
 * the entire 8 bit Alpha component.
 * \n\n
 * <code>
 * RGBA (32bit):   r7r6r5r4r3r2r1r0|g7g6g5g4g3g2g1g0|b7b6b5b4b3b2b1b|0a7a6a5a4a3a2a1a0
 * \n
 * RGBIA8 (16bit): b7b6b5b4b3b2b1b0|a7a6a5a4a3a2a1a0
 * </code>
 *
 * @param rgba	A 32-bit RGBA value to convert to the IA8 format.
 * @return The IA8 value of the given RGBA value.
 */

constexpr uint16_t convertRGBAToIA8(uint32_t rgba) {

	return RGBA_TO_IA8(rgba);
}


/**
 * Downsample the specified RGBA value data buffer to an RGB565 value.
 *
 * This routine downsamples the given RGBA data value into the RGB565 texture data format.
 * Attribution for this routine is given fully to NoNameNo of GRRLIB Wii library.
 *
 * <strong>Format Explanation</strong>
 * \n
 * RGB565 is a color format without an Alpha component. In order to convert from the given RGBA color format simply concatenate
 * the first 5 MSB from the Red component to the first 6 MSB from the Green component to the first 5 MSB from the Blue component.
 * \n\n
 * <code>
 * RGBA (32bit):   r7r6r5r4r3r2r1r0|g7g6g5g4g3g2g1g0|b7b6b5b4b3b2b1b|0a7a6a5a4a3a2a1a0
 * \n
 * RGB565 (16bit): r7r6r5r4r3g7g6g5|g4g3g2b7b6b5b4b3
 * </code>
 *
 * @param rgba	A 32-bit RGBA value to convert to the RGB565 format.
 * @return The RGB565 value of the given RGBA value.
 */

constexpr uint16_t convertRGBAToRGB565(uint32_t rgba) {
	return RGBA_TO_RGB565(rgba);
}


/**
 * Downsample the specified RGBA value data buffer to an RGB5A3 value.
 *
 * This routine downsamples the given RGBA data value into the RGB5A3 texture data format.
 *
 * <strong>Format Explanation</strong>
 * \n
 * RGB5A3 is really a conditional application of the RGB444 and RGB555 formats based off of the value of the Alpha channel.
 * If the 3 MSB of the Alpha channel is fully opaque (a7a6a5 == [1][1][1]) then the RGBA color is converted to RGB555 with the first bit set to 1.
 * Otherwise the RGBA color is converted to RGB444 with a 3 bit alpha channel and the first bit set to 0.
 *\n\n
 * <code>
 * RGBA (32bit):    r7r6r5r4r3r2r1r0|g7g6g5g4g3g2g1g0|b7b6b5b4b3b2b1b|0a7a6a5a4a3a2a1a0
 * \n
 * RGB555 (16bit): [1]r7r6r5r4r3g7g6|g5g4g3b7b6b5b4b3
 * \n
 * RGB444 (16bit): [0]r7r6r5r4g7g6g5|g4b7b6b5b4a7a6a5
 * </code>
 *
 * @param rgba	A 32-bit RGBA value to convert to the RGB5A3 format.
 * @return The RGB5A3 value of the given RGBA value.
 */

constexpr uint16_t convertRGBAToRGB5A3(uint32_t rgba) {
	return RGBA_TO_RGB5A3(rgba);
}
