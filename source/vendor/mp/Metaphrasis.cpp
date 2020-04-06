// This is a cleaned up port of "Metaphrasis" by Armin Tamzarian for PC (https://github.com/ArminTamzarian/metaphrasis/blob/master/Metaphrasis.cpp)

#include "Metaphrasis.h"

#include <cstdlib>


namespace {
inline uint32_t* memalign(uint32_t align, uint32_t size)
{
	return (uint32_t*)new char[size];
}
}

/**
 * Convert the specified RGBA data buffer into the I4 texture format
 * 
 * This routine converts the RGBA data buffer into the I4 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToI4(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = bufferWidth * bufferHeight >> 1;
	uint32_t* dataBufferI4 = new uint32_t[bufferSize/4];
	memset(dataBufferI4, 0x00, bufferSize);

	uint32_t *src = (uint32_t *)rgbaBuffer;
	uint8_t *dst = (uint8_t *)dataBufferI4;

	for(uint16_t y = 0; y < bufferHeight; y += 8) {
		for(uint16_t x = 0; x < bufferWidth; x += 8) {
			for(uint16_t rows = 0; rows < 8; rows++) {
				*dst++ = (src[((y + rows) * bufferWidth) + (x + 0)] & 0xf0) | ((src[((y + rows) * bufferWidth) + (x + 1)] & 0xf0) >> 4);
				*dst++ = (src[((y + rows) * bufferWidth) + (x + 2)] & 0xf0) | ((src[((y + rows) * bufferWidth) + (x + 3)] & 0xf0) >> 4);
				*dst++ = (src[((y + rows) * bufferWidth) + (x + 4)] & 0xf0) | ((src[((y + rows) * bufferWidth) + (x + 5)] & 0xf0) >> 4);
				*dst++ = (src[((y + rows) * bufferWidth) + (x + 6)] & 0xf0) | ((src[((y + rows) * bufferWidth) + (x + 7)] & 0xf0) >> 4);
			}
		}
	}
	
	return std::unique_ptr<uint32_t[]>(dataBufferI4);
}

/**
 * Convert the specified RGBA data buffer into the I8 texture format
 * 
 * This routine converts the RGBA data buffer into the I8 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToI8(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = bufferWidth * bufferHeight;
	uint32_t* dataBufferI8 = new uint32_t[bufferSize / 4];
	memset(dataBufferI8, 0x00, bufferSize);

	uint32_t *src = (uint32_t *)rgbaBuffer;
	uint8_t *dst = (uint8_t *)dataBufferI8;

	for(uint16_t y = 0; y < bufferHeight; y += 4) {
		for(uint16_t x = 0; x < bufferWidth; x += 8) {
			for(uint16_t rows = 0; rows < 4; rows++) {
				*dst++ = src[((y + rows) * bufferWidth) + (x + 0)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 1)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 2)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 3)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 4)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 5)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 6)] & 0xff;
				*dst++ = src[((y + rows) * bufferWidth) + (x + 7)] & 0xff;
			}
		}
	}
	
	return std::unique_ptr<uint32_t[]>(dataBufferI8);
}


/**
 * Convert the specified RGBA data buffer into the IA4 texture format
 * 
 * This routine converts the RGBA data buffer into the IA4 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToIA4(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = bufferWidth * bufferHeight;
	uint32_t* dataBufferIA4 = new uint32_t[bufferSize / 4];
	memset(dataBufferIA4, 0x00, bufferSize);

	uint32_t *src = (uint32_t *)rgbaBuffer;
	uint8_t *dst = (uint8_t *)dataBufferIA4;

	for(uint16_t y = 0; y < bufferHeight; y += 4) {
		for(uint16_t x = 0; x < bufferWidth; x += 8) {
			for(uint16_t rows = 0; rows < 4; rows++) {
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 0)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 1)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 2)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 3)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 4)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 5)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 6)]);
				*dst++ = RGBA_TO_IA4(src[((y + rows) * bufferWidth) + (x + 7)]);
			}
		}
	}

	return std::unique_ptr<uint32_t[]>(dataBufferIA4);
}

/**
 * Convert the specified RGBA data buffer into the IA8 texture format
 * 
 * This routine converts the RGBA data buffer into the IA8 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToIA8(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = (bufferWidth * bufferHeight) << 1;
	uint32_t* dataBufferIA8 = new uint32_t[bufferSize / 4];
	memset(dataBufferIA8, 0x00, bufferSize);

	uint32_t *src = (uint32_t *)rgbaBuffer;
	uint16_t *dst = (uint16_t *)dataBufferIA8;

	for(uint16_t y = 0; y < bufferHeight; y += 4) {
		for(uint16_t x = 0; x < bufferWidth; x += 4) {
			for(uint16_t rows = 0; rows < 4; rows++) {
				*dst++ = RGBA_TO_IA8(src[((y + rows) * bufferWidth) + (x + 0)]);
				*dst++ = RGBA_TO_IA8(src[((y + rows) * bufferWidth) + (x + 1)]);
				*dst++ = RGBA_TO_IA8(src[((y + rows) * bufferWidth) + (x + 2)]);
				*dst++ = RGBA_TO_IA8(src[((y + rows) * bufferWidth) + (x + 3)]);
			}
		}
	}

	return std::unique_ptr<uint32_t[]>(dataBufferIA8);
}

/**
 * Convert the specified RGBA data buffer into the RGBA8 texture format
 * 
 * This routine converts the RGBA data buffer into the RGBA8 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToRGBA8(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = (bufferWidth * bufferHeight) << 2;
	uint32_t* dataBufferRGBA8 = new uint32_t[bufferSize / 4];
	memset(dataBufferRGBA8, 0x00, bufferSize);

	uint8_t *src = (uint8_t *)rgbaBuffer;
	uint8_t *dst = (uint8_t *)dataBufferRGBA8;

	for(uint16_t block = 0; block < bufferHeight; block += 4) {
		for(uint16_t i = 0; i < bufferWidth; i += 4) {
            for (uint32_t c = 0; c < 4; c++) {
				uint32_t blockWid = (((block + c) * bufferWidth) + i) << 2 ;

				*dst++ = src[blockWid + 3];  // ar = 0
				*dst++ = src[blockWid + 0];
				*dst++ = src[blockWid + 7];  // ar = 1
				*dst++ = src[blockWid + 4];
				*dst++ = src[blockWid + 11]; // ar = 2
				*dst++ = src[blockWid + 8];
				*dst++ = src[blockWid + 15]; // ar = 3
				*dst++ = src[blockWid + 12];
            }
            for (uint32_t c = 0; c < 4; c++) {
				uint32_t blockWid = (((block + c) * bufferWidth) + i ) << 2 ;

				*dst++ = src[blockWid + 1];  // gb = 0
				*dst++ = src[blockWid + 2];
				*dst++ = src[blockWid + 5];  // gb = 1
				*dst++ = src[blockWid + 6];
				*dst++ = src[blockWid + 9];  // gb = 2
				*dst++ = src[blockWid + 10];
				*dst++ = src[blockWid + 13]; // gb = 3
				*dst++ = src[blockWid + 14];
            }
		}
	}

	return std::unique_ptr<uint32_t[]>(dataBufferRGBA8);
}

/**
 * Convert the specified RGBA data buffer into the RGB565 texture format
 * 
 * This routine converts the RGBA data buffer into the RGB565 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToRGB565(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = (bufferWidth * bufferHeight) << 1;
	uint32_t* dataBufferRGB565 = new uint32_t[bufferSize / 4];
	memset(dataBufferRGB565, 0x00, bufferSize);

	uint32_t *src = (uint32_t *)rgbaBuffer;
	uint16_t *dst = (uint16_t *)dataBufferRGB565;

	for(uint16_t y = 0; y < bufferHeight; y += 4) {
		for(uint16_t x = 0; x < bufferWidth; x += 4) {
			for(uint16_t rows = 0; rows < 4; rows++) {
				*dst++ = RGBA_TO_RGB565(src[((y + rows) * bufferWidth) + (x + 0)]);
				*dst++ = RGBA_TO_RGB565(src[((y + rows) * bufferWidth) + (x + 1)]);
				*dst++ = RGBA_TO_RGB565(src[((y + rows) * bufferWidth) + (x + 2)]);
				*dst++ = RGBA_TO_RGB565(src[((y + rows) * bufferWidth) + (x + 3)]);
			}
		}
	}

	return std::unique_ptr<uint32_t[]>(dataBufferRGB565);
}

	
/**
 * Convert the specified RGBA data buffer into the RGB5A3 texture format
 * 
 * This routine converts the RGBA data buffer into the RGB5A3 texture format and returns a pointer to the converted buffer.
 * 
 * @param rgbaBuffer	Buffer containing the temporarily rendered RGBA data.
 * @param bufferWidth	Pixel width of the data buffer.
 * @param bufferHeight	Pixel height of the data buffer.
 * @return	A pointer to the allocated buffer.
 */

std::unique_ptr<uint32_t[]> convertBufferToRGB5A3(uint32_t* rgbaBuffer, uint16_t bufferWidth, uint16_t bufferHeight) {
	uint32_t bufferSize = (bufferWidth * bufferHeight) << 1;
	uint32_t* dataBufferRGB5A3 = new uint32_t[bufferSize / 4];
	memset(dataBufferRGB5A3, 0x00, bufferSize);

	uint32_t *src = (uint32_t *)rgbaBuffer;
	uint16_t *dst = (uint16_t *)dataBufferRGB5A3;

	for(uint16_t y = 0; y < bufferHeight; y += 4) {
		for(uint16_t x = 0; x < bufferWidth; x += 4) {
			for(uint16_t rows = 0; rows < 4; rows++) {
				*dst++ = RGBA_TO_RGB5A3(src[((y + rows) * bufferWidth) + (x + 0)]);
				*dst++ = RGBA_TO_RGB5A3(src[((y + rows) * bufferWidth) + (x + 1)]);
				*dst++ = RGBA_TO_RGB5A3(src[((y + rows) * bufferWidth) + (x + 2)]);
				*dst++ = RGBA_TO_RGB5A3(src[((y + rows) * bufferWidth) + (x + 3)]);
			}
		}
	}

	return std::unique_ptr<uint32_t[]>(dataBufferRGB5A3);
}
