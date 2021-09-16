#pragma once

#include <stdint.h>

typedef uint8_t BYTE;
typedef uint32_t DWORD;

typedef struct RGB_ {
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
} RGB;

DWORD reduce(DWORD col);

int lightnessCompare(const void *d1, const void *d2);

void createPaletteExact(DWORD *img, int width, int height, DWORD *pal, unsigned int nColors);

void createPalette_(DWORD * img, int width, int height, DWORD * pal, int nColors);

closestpalette(RGB rgb, RGB * palette, int paletteSize, RGB * error);

void doDiffuse(int i, int width, int height, unsigned int * pixels, int errorRed, int errorGreen, int errorBlue, int errorAlpha, float amt);

DWORD averageColor(DWORD *cols, int nColors);

unsigned int getPaletteError(RGB *px, int nPx, RGB *pal, int paletteSize);

void createMultiplePalettes(DWORD *blocks, DWORD *avgs, int width, int tilesX, int tilesY, DWORD *pals, int nPalettes, int paletteSize, int usePaletteSize, int paletteOffset);

void convertRGBToYUV(int r, int g, int b, int *y, int *u, int *v);
