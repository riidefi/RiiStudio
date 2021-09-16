#include "palette.h"
#include "color.h"
#include "texconv.h"
#include "analysis.h"
#include <stdlib.h>
#include <string.h>

#include <math.h>

int ilog2(int x);

#ifndef _WIN32

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#endif

int convertDirect(CREATEPARAMS *params) {
	//convert to direct color.
	int width = params->width, height = params->height;
	DWORD *px = params->px;

	params->dest->texels.texImageParam = (ilog2(width >> 3) << 20) | (ilog2(height >> 3) << 23) | (params->fmt << 26);
	if (params->dest->texels.cmp) free(params->dest->texels.cmp);
	params->dest->texels.cmp = NULL;
	if (params->dest->texels.texel) free(params->dest->texels.texel);
	if (params->dest->palette.pal) free(params->dest->palette.pal);
	params->dest->palette.pal = NULL;
	params->dest->palette.nColors = 0;
	COLOR *txel = (COLOR *) calloc(width * height, 2);
	params->dest->texels.texel = (char *) txel;
	for (int i = 0; i < width * height; i++) {
		DWORD p = px[i];
		COLOR c = ColorConvertToDS(p);
		if (px[i] & 0xFF000000) c |= 0x8000;
		if (params->dither) {
			DWORD back = ColorConvertFromDS(c);
			int errorRed = (back & 0xFF) - (p & 0xFF);
			int errorGreen = ((back >> 8) & 0xFF) - ((p >> 8) & 0xFF);
			int errorBlue = ((back >> 16) & 0xFF) - ((back >> 16) & 0xFF);
			doDiffuse(i, width, height, px, -errorRed, -errorGreen, -errorBlue, 0, 1.0f);
		}
		txel[i] = c;
	}
	return 0;
}

int convertPalette(CREATEPARAMS *params) {
	//convert to translucent. First, generate a palette of colors.
	int nColors = 0, bitsPerPixel = 0;
	int width = params->width, height = params->height;
	switch (params->fmt) {
		case CT_4COLOR:
			nColors = 4;
			bitsPerPixel = 2;
			break;
		case CT_16COLOR:
			nColors = 16;
			bitsPerPixel = 4;
			break;
		case CT_256COLOR:
			nColors = 256;
			bitsPerPixel = 8;
			break;
	}
	int pixelsPerByte = 8 / bitsPerPixel;
	if (params->useFixedPalette) nColors = min(nColors, params->colorEntries);
	DWORD *palette = (DWORD *) calloc(nColors, 4);

	//should we reserve a color for transparent?
	int hasTransparent = 0;
	for (int i = 0; i < width * height; i++) {
		if ((params->px[i] & 0xFF000000) == 0) {
			hasTransparent = 1;
			break;
		}
	}

	if (!params->useFixedPalette) {
		//generate a palette, making sure to leave a transparent color, if applicable.
		createPaletteExact(params->px, width, height, palette + hasTransparent, nColors - hasTransparent);
	} else {
		for (int i = 0; i < nColors; i++) {
			palette[i] = ColorConvertFromDS(params->fixedPalette[i]);
		}
	}

	//allocate texel space.
	int nBytes = width * height * bitsPerPixel / 8;
	BYTE *txel = (BYTE *) calloc(nBytes, 1);

	//write texel data.
	for (int i = 0; i < width * height; i++) {
		DWORD p = params->px[i];
		int index = 0;
		RGB error;
		if (p & 0xFF000000) index = closestpalette(*(RGB *) &p, (RGB *) palette + hasTransparent, nColors - hasTransparent, &error) + hasTransparent;
		txel[i / pixelsPerByte] |= index << (bitsPerPixel * (i & (pixelsPerByte - 1)));
		if (params->dither) {				
			DWORD back = palette[index];
			int errorRed = (back & 0xFF) - (p & 0xFF);
			int errorGreen = ((back >> 8) & 0xFF) - ((p >> 8) & 0xFF);
			int errorBlue = ((back >> 16) & 0xFF) - ((p >> 16) & 0xFF);
			doDiffuse(i, width, height, params->px, -errorRed, -errorGreen, -errorBlue, 0, 1.0f);
		}
	}

	//update texture info
	if (params->dest->palette.pal) free(params->dest->palette.pal);
	if (params->dest->texels.cmp) free(params->dest->texels.cmp);
	if (params->dest->texels.texel) free(params->dest->texels.texel);
	params->dest->palette.nColors = nColors;
	params->dest->palette.pal = (COLOR *) calloc(nColors, 2);
	params->dest->texels.cmp = NULL;
	params->dest->texels.texel = txel;
	memcpy(params->dest->palette.name, params->pnam, 16);

	unsigned int param = (params->fmt << 26) | (ilog2(width >> 3) << 20) | (ilog2(height >> 3) << 23);
	if (hasTransparent) param |= (1 << 29);
	params->dest->texels.texImageParam = param;

	for (int i = 0; i < nColors; i++) {
		params->dest->palette.pal[i] = ColorConvertToDS(palette[i]);
	}
	free(palette);
	return 0;
}

int convertTranslucent(CREATEPARAMS *params) {
	//convert to translucent. First, generate a palette of colors.
	int nColors = 0, alphaShift = 0, alphaMax = 0;
	int width = params->width, height = params->height;
	switch (params->fmt) {
		case CT_A3I5:
			nColors = 32;
			alphaShift = 5;
			alphaMax = 7;
			break;
		case CT_A5I3:
			nColors = 8;
			alphaShift = 3;
			alphaMax = 31;
			break;
	}
	if (params->useFixedPalette) nColors = min(nColors, params->colorEntries);
	DWORD *palette = (DWORD *) calloc(nColors, 4);

	if (!params->useFixedPalette) {
		//generate a palette, making sure to leave a transparent color, if applicable.
		createPaletteExact(params->px, width, height, palette, nColors);
	} else {
		for (int i = 0; i < nColors; i++) {
			palette[i] = ColorConvertFromDS(params->fixedPalette[i]);
		}
	}

	//allocate texel space.
	int nBytes = width * height;
	BYTE *txel = (BYTE *) calloc(nBytes, 1);

	//write texel data.
	for (int i = 0; i < width * height; i++) {
		DWORD p = params->px[i];
		RGB error;
		int index = closestpalette(*(RGB *) &p, (RGB *) palette, nColors, &error);
		int alpha = (((p >> 24) & 0xFF) * alphaMax + 127) / 255;
		txel[i] = index | (alpha << alphaShift);
		if (params->dither) {				
			DWORD back = palette[index];
			int errorRed = (back & 0xFF) - (p & 0xFF);
			int errorGreen = ((back >> 8) & 0xFF) - ((p >> 8) & 0xFF);
			int errorBlue = ((back >> 16) & 0xFF) - ((p >> 16) & 0xFF);
			int errorAlpha = ((back >> 24) & 0xFF) - ((p >> 24) & 0xFF);
			doDiffuse(i, width, height, params->px, -errorRed, -errorGreen, -errorBlue, params->ditherAlpha ? -errorAlpha : 0, 1.0f);
		}
	}

	//update texture info
	if (params->dest->palette.pal) free(params->dest->palette.pal);
	if (params->dest->texels.cmp) free(params->dest->texels.cmp);
	if (params->dest->texels.texel) free(params->dest->texels.texel);
	params->dest->palette.nColors = nColors;
	params->dest->palette.pal = (COLOR *) calloc(nColors, 2);
	params->dest->texels.cmp = NULL;
	params->dest->texels.texel = txel;
	memcpy(params->dest->palette.name, params->pnam, 16);

	unsigned int param = (params->fmt << 26) | (ilog2(width >> 3) << 20) | (ilog2(height >> 3) << 23);
	params->dest->texels.texImageParam = param;

	for (int i = 0; i < nColors; i++) {
		params->dest->palette.pal[i] = ColorConvertToDS(palette[i]);
	}
	free(palette);
	return 0;
}

//blend two colors together by weight.
DWORD blend(DWORD col1, int weight1, DWORD col2, int weight2) {
	int r1 = col1 & 0xFF;
	int g1 = (col1 >> 8) & 0xFF;
	int b1 = (col1 >> 16) & 0xFF;
	int r2 = col2 & 0xFF;
	int g2 = (col2 >> 8) & 0xFF;
	int b2 = (col2 >> 16) & 0xFF;
	int r3 = (r1 * weight1 + r2 * weight2) >> 3;
	int g3 = (g1 * weight1 + g2 * weight2) >> 3;
	int b3 = (b1 * weight1 + b2 * weight2) >> 3;
	return r3 | (g3 << 8) | (b3 << 16);
}

volatile int _globColors = 0;
volatile int _globFinal = 0;
volatile int _globFinished = 0;

typedef struct {
	BYTE rgb[64];
	WORD used;
	WORD mode;
	COLOR palette[4];
	WORD paletteIndex;
	BYTE transparentPixels;
	BYTE duplicate;
} TILEDATA;

int pixelCompare(const void *p1, const void *p2) {
	return *(DWORD *) p1 - (*(DWORD *) p2);
}

int countColors(DWORD *px, int nPx) {
	//sort the colors by raw RGB value. This way, same colors are grouped.
	DWORD *copy = (DWORD *) malloc(nPx * 4);
	memcpy(copy, px, nPx * 4);
	qsort(copy, nPx, 4, pixelCompare);
	int nColors = 0;
	int hasTransparent = 0;
	for (int i = 0; i < nPx; i++) {
		int a = copy[i] >> 24;
		if (!a) hasTransparent = 1;
		else {
			DWORD color = copy[i] & 0xFFFFFF;
			//has this color come before?
			int repeat = 0;
			if(i){
				DWORD comp = copy[i - 1] & 0xFFFFFF;
				if (comp == color) {
					repeat = 1;
				}
			}
			if (!repeat) {
				nColors++;
			}
		}
	}
	free(copy);
	return nColors + hasTransparent;
}

void getColorBounds(DWORD *px, int nPx, DWORD *colorMin, DWORD *colorMax) {
	//if only 1 or 2 colors, fill the palette with those.
	
	DWORD colors[2];
	int nColors = 0;
	for (int i = 0; i < nPx; i++) {
		if (px[i] >> 24 < 0x80) continue;
		if (nColors == 0) {
			colors[0] = px[i];
			nColors++;
		} else if (nColors == 1) {
			colors[1] = px[i];
			nColors++;
		} else {
			nColors++;
			break;
		}
	}
	if (nColors <= 2) {
		if (nColors == 0) {
			*colorMin = 0;
			*colorMax = 0;
		} else if (nColors == 1) {
			*colorMin = colors[0];
			*colorMax = colors[1];
		} else {
			int y1, u1, v1, y2, u2, v2;
			convertRGBToYUV(colors[0] & 0xFF, (colors[0] >> 8) & 0xFF, (colors[0] >> 16) & 0xFF, &y1, &u1, &v1);
			convertRGBToYUV(colors[1] & 0xFF, (colors[1] >> 8) & 0xFF, (colors[1] >> 16) & 0xFF, &y2, &u2, &v2);
			if (y1 > y2) {
				*colorMin = colors[1];
				*colorMax = colors[0];
			} else {
				*colorMin = colors[0];
				*colorMax = colors[1];
			}
		}
		return;
	}

	DWORD cols[2];
	getColorEndPoints(px, nPx, cols);
	*colorMin = cols[0];
	*colorMax = cols[1];

}

int computeColorDifference(DWORD c1, DWORD c2) {
	int r1 = c1 & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = (c1 >> 16) & 0xFF;
	int r2 = c2 & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = (c2 >> 16) & 0xFF;

	int dy, du, dv;
	//property of linear transformations :)
	convertRGBToYUV(r2 - r1, g2 - g1, b2 - b1, &dy, &du, &dv);

	return 4 * dy * dy + du * du + dv * dv;
}

//compute LMS squared
int computeLMS(DWORD *tile, DWORD *palette, int transparent) {
	int total = 0, nCount = 0;
	for (int i = 0; i < 16; i++) {
		DWORD c = tile[i];
		if (!transparent || (c >> 24) >= 0x80) {
			int closest = closestpalette(*(RGB *) &c, (RGB *) palette, 4 - transparent, NULL);
			DWORD chosen = palette[closest];
			total += computeColorDifference(c, chosen);
			nCount++;
		}
	}
	if (nCount == 0) return 0;
	return total / nCount;
}

void choosePaletteAndMode(TILEDATA *tile) {
	//first try interpolated. If it's not good enough, use full color.
	DWORD colorMin, colorMax;
	getColorBounds((DWORD *) tile->rgb, 16, &colorMin, &colorMax);
	if (tile->transparentPixels) {
		DWORD mid = blend(colorMin, 4, colorMax, 4);
		DWORD palette[] = { colorMax, mid, colorMin, 0 };
		int error = computeLMS((DWORD *) tile->rgb, palette, 1);
		//if error <= 24, then these colors are good enough
		if (error <= 24) {
			tile->palette[0] = ColorConvertToDS(colorMax);
			tile->palette[1] = ColorConvertToDS(colorMin);
			tile->palette[2] = 0;
			tile->palette[3] = 0;
			tile->mode = 0x4000;
		} else {
			createPaletteExact((DWORD *) tile->rgb, 4, 4, palette, 4);
			//palette[0] = 0;
			//swap index 3 and 0, 2 and 1
			tile->palette[0] = ColorConvertToDS(palette[3]);
			tile->palette[1] = ColorConvertToDS(palette[2]);
			tile->palette[2] = ColorConvertToDS(palette[1]);
			tile->palette[3] = ColorConvertToDS(palette[0]);
			tile->mode = 0x0000;
		}
	} else {
		DWORD mid1 = blend(colorMin, 5, colorMax, 3);
		DWORD mid2 = blend(colorMin, 3, colorMax, 5);
		DWORD palette[] = { colorMax, mid2, mid1, colorMin };
		int error = computeLMS((DWORD *) tile->rgb, palette, 0);
		if (error <= 24) {
			tile->palette[0] = ColorConvertToDS(colorMax);
			tile->palette[1] = ColorConvertToDS(colorMin);
			tile->palette[2] = 0;
			tile->palette[3] = 0;
			tile->mode = 0xC000;
		} else {
			createPaletteExact((DWORD *) tile->rgb, 4, 4, (DWORD *) palette, 4);
			//swap index 3 and 0, 2 and 1
			tile->palette[0] = ColorConvertToDS(palette[3]);
			tile->palette[1] = ColorConvertToDS(palette[2]);
			tile->palette[2] = ColorConvertToDS(palette[1]);
			tile->palette[3] = ColorConvertToDS(palette[0]);;
			tile->mode = 0x8000;
		}
	}
}

void addTile(TILEDATA *data, int index, DWORD *px, int *totalIndex) {
	memcpy(data[index].rgb, px, 64);
	data[index].duplicate = 0;
	data[index].used = 1;
	data[index].transparentPixels = 0;
	data[index].mode = 0;
	data[index].paletteIndex = 0;

	//count transparent pixels
	int nTransparentPixels = 0;
	for (int i = 0; i < 16; i++) {
		DWORD c = px[i];
		int a = (c >> 24) & 0xFF;
		if (a < 0x80) nTransparentPixels++;
	}
	data[index].transparentPixels = nTransparentPixels;
	
	//is it a duplicate?
	int isDuplicate = 0;
	int duplicateIndex = 0;
	for (int i = 0; i < index; i++) {
		TILEDATA *tile = data + i;
		DWORD *px1 = (DWORD *) tile->rgb;
		DWORD *px2 = (DWORD *) data[index].rgb;
		int same = 1;
		for (int j = 0; j < 16; j++) {
			if (px1[j] != px2[j]) {
				same = 0;
				break;
			}
		}
		if (same) {
			isDuplicate = 1;
			duplicateIndex = i;
			break;
		}
	}

	if (isDuplicate) {
		memcpy(data + index, data + duplicateIndex, sizeof(TILEDATA));
		data[index].duplicate = 1;
		data[index].paletteIndex = data[duplicateIndex].paletteIndex;
	} else {
		//generate a palette and determine the mode.
		choosePaletteAndMode(data + index);
		data[index].paletteIndex = *totalIndex;
		//is the palette and mode identical to a non-duplicate tile?
		for (int i = 0; i < index; i++) {
			TILEDATA *tile1 = data + i;
			TILEDATA *tile2 = data + index;
			if (tile1->duplicate) continue;
			if (tile1->mode != tile2->mode) continue;
			if (tile1->palette[0] != tile2->palette[0] || tile1->palette[1] != tile2->palette[1]) continue;
			if (tile1->mode == 0x0000 || tile1->mode == 0x8000) {
				if (tile1->palette[2] != tile2->palette[2] || tile1->palette[3] != tile2->palette[3]) continue;
			}
			//palettes and modes are the same, mark as duplicate.
			tile2->duplicate = 1;
			tile2->paletteIndex = tile1->paletteIndex;
			break;
		}
	}
	if (!data[index].duplicate) {
		int nPalettes = 1;
		if (data[index].mode == 0x8000 || data[index].mode == 0x0000) {
			nPalettes = 2;
		}
		*totalIndex += nPalettes;
	}
	_globColors++;
}

TILEDATA *createTileData(DWORD *px, int tilesX, int tilesY) {
	TILEDATA *data = (TILEDATA *) calloc(tilesX * tilesY, sizeof(TILEDATA));
	int paletteIndex = 0;
	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			DWORD tile[16];
			int offs = x * 4 + y * 4 * tilesX * 4;
			memcpy(tile, px + offs, 16);
			memcpy(tile + 4, px + offs + tilesX * 4, 16);
			memcpy(tile + 8, px + offs + tilesX * 8, 16);
			memcpy(tile + 12, px + offs + tilesX * 12, 16);
			addTile(data, x + y * tilesX, tile, &paletteIndex);
		}
	}
	return data;
}

int getColorsFromTable(BYTE type) {
	if (type == 0) return 0;
	if (type == 2 || type == 8) return 2;
	return 4;
}

WORD getModeFromTable(BYTE type) {
	if (type == 1) return 0x0000;
	if (type == 2) return 0x4000;
	if (type == 4) return 0x8000;
	if (type == 8) return 0xC000;
	return 0;
}

int computePaletteDifference(DWORD *pal1, DWORD *pal2, int nColors) {
	int total = 0;
	
	for (int i = 0; i < nColors; i++) {
		total += computeColorDifference(pal1[i], pal2[i]);
	}

	if (nColors == 2) total *= 2;
	return total;
}

int findClosestPalettes(COLOR *palette, BYTE *colorTable, int nColors, int *colorIndex1, int *colorIndex2) {
	//determine which two palettes are the most similar. For 2-color palettes, multiply difference by 2.
	int leastDistance = 0x10000000;
	int idx1 = 0;

	while (idx1 <= nColors) {
		BYTE type1 = colorTable[idx1];
		if (type1 == 0) break;
		int nColorsInThisPalette = 2;
		if (type1 == 4 || type1 == 1) {
			nColorsInThisPalette = 4;
		}

		//start searching forward.
		int idx2 = idx1 + nColorsInThisPalette;
		while (idx2 + nColorsInThisPalette <= nColors) {
			BYTE type2 = colorTable[idx2];
			if (!type2) break;
			int nColorsInSecondPalette = getColorsFromTable(type2);
			if (type2 != type1) {
				idx2 += nColorsInSecondPalette;
				continue;
			}

			//same type, let's compare.
			DWORD expandPal1[4], expandPal2[4];
			WORD mode = getModeFromTable(type1);
			for (int i = 0; i < nColorsInThisPalette; i++) {
				expandPal1[i] = ColorConvertFromDS(palette[idx1 + i]);
				expandPal2[i] = ColorConvertFromDS(palette[idx2 + i]);
			}
			int dst = computePaletteDifference(expandPal1, expandPal2, nColorsInThisPalette);
			if (dst < leastDistance) {
				leastDistance = dst;
				*colorIndex1 = idx1;
				*colorIndex2 = idx2;
				if (!leastDistance) return 0;
			}

			idx2 += nColorsInThisPalette;
		}
		idx1 += nColorsInThisPalette;

	}
	return leastDistance;
}

void mergePalettes(TILEDATA *tileData, int nTiles, COLOR *palette, int paletteIndex, WORD palettesMode) {
	//count the number of tiles that use this palette.
	int nUsedTiles = 0;
	for (int i = 0; i < nTiles; i++) {
		if (tileData[i].paletteIndex == paletteIndex) nUsedTiles++;
	}

	//allocate space for all of the color data
	DWORD *px = (DWORD *) calloc(nUsedTiles, 16 * 4);

	//copy tiles into the buffer
	int copiedTiles = 0;
	for (int i = 0; i < nTiles; i++) {
		if (tileData[i].paletteIndex == paletteIndex) {
			memcpy(px + copiedTiles * 16, tileData[i].rgb, 16 * 4);
			copiedTiles++;
		}
	}

	//use the mode to determine the appropriate method of creating the palette.
	DWORD expandPal[4];
	if (palettesMode == 0x0000) {
		//transparent, full color
		createPalette_(px, nUsedTiles, 16, expandPal, 4);
		expandPal[0] = 0;
		palette[paletteIndex * 2 + 0] = ColorConvertToDS(expandPal[3]);
		palette[paletteIndex * 2 + 1] = ColorConvertToDS(expandPal[2]);
		palette[paletteIndex * 2 + 2] = ColorConvertToDS(expandPal[1]);
		palette[paletteIndex * 2 + 3] = 0;
	} else if (palettesMode == 0x4000 || palettesMode == 0xC000) {
		//transparent, interpolated, and opaque, interpolated
		//just average all the palettes consumed.
		int r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0;
		for (int i = 0; i < nTiles; i++) {
			if (tileData[i].paletteIndex != paletteIndex) continue;
			DWORD c1 = ColorConvertFromDS(tileData[i].palette[0]), c2 = ColorConvertFromDS(tileData[i].palette[1]);
			r1 += c1 & 0xFF;
			r2 += c2 & 0xFF;
			g1 += (c1 >> 8) & 0xFF;
			g2 += (c2 >> 8) & 0xFF;
			b1 += (c1 >> 16) & 0xFF;
			b2 += (c2 >> 16) & 0xFF;
		}
		r1 = (r1 + (nUsedTiles >> 1)) / nUsedTiles;
		g1 = (g1 + (nUsedTiles >> 1)) / nUsedTiles;
		b1 = (b1 + (nUsedTiles >> 1)) / nUsedTiles;
		r2 = (r2 + (nUsedTiles >> 1)) / nUsedTiles;
		g2 = (g2 + (nUsedTiles >> 1)) / nUsedTiles;
		b2 = (b2 + (nUsedTiles >> 1)) / nUsedTiles;
		palette[paletteIndex * 2 + 0] = ColorConvertToDS(r1 | (g1 << 8) | (b1 << 16));
		palette[paletteIndex * 2 + 1] = ColorConvertToDS(r2 | (g2 << 8) | (b2 << 16));
	} else if (palettesMode == 0x8000) {
		//opaque, full color
		createPaletteExact(px, nUsedTiles, 16, expandPal, 4);
		palette[paletteIndex * 2 + 0] = ColorConvertToDS(expandPal[3]);
		palette[paletteIndex * 2 + 1] = ColorConvertToDS(expandPal[2]);
		palette[paletteIndex * 2 + 2] = ColorConvertToDS(expandPal[1]);
		palette[paletteIndex * 2 + 3] = ColorConvertToDS(expandPal[0]);
	}
	free(px);
}

int buildPalette(COLOR *palette, int nPalettes, TILEDATA *tileData, int tilesX, int tilesY, int threshold) {
	//iterate over all non-duplicate tiles, adding the palettes.
	//colorTable keeps track of how each color is intended to be used.
	//00 - unused. 01 - mode 0x0000. 02 - mode 0x4000. 04 - mode 0x8000. 08 - mode 0xC000.
	BYTE *colorTable = (BYTE *) calloc(nPalettes * 2, 1);
	int firstSlot = 0;
	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			int index = x + y * tilesX;
			TILEDATA *tile = tileData + index;
			if (tile->duplicate) {
				//the paletteIndex field of a duplicate tile is first set to the tile index it is a duplicate of.
				//set it to an actual palette index here.
				_globColors++;
				continue;
			}

			//how many color entries does this consume?
			int nConsumed = 4;
			if (tile->mode == 0x4000) nConsumed = 2;
			if (tile->mode == 0xC000) nConsumed = 2;

			//does it fit?
			int fits = 0;
			if (firstSlot + nConsumed <= nPalettes * 2) {
				//yes, just add it to the list.
				fits = 1;
				memcpy(palette + firstSlot, tile->palette, nConsumed * sizeof(COLOR));
				BYTE fill = 1 << (tile->mode >> 14);
				memset(colorTable + firstSlot, fill, nConsumed);
				tile->paletteIndex = firstSlot / 2;
				firstSlot += nConsumed;
			}
			if(!fits || (threshold && firstSlot >= 8)) {
				//does NOT fit, we need to rearrange some things.

				while ((firstSlot + nConsumed > nPalettes * 2) || (threshold && fits)) {
				//determine which two palettes are the most similar.
					int colorIndex1 = -1, colorIndex2 = -1;
					int distance = findClosestPalettes(palette, colorTable, firstSlot, &colorIndex1, &colorIndex2);
					if (colorIndex1 == -1) break;
					if (fits && (distance > threshold * 10 || firstSlot < 8)) break;
					int nColorsInPalettes = getColorsFromTable(colorTable[colorIndex1]);
					WORD palettesMode = getModeFromTable(colorTable[colorIndex1]);

					//find tiles that use colorIndex2. Set them to use colorIndex1. 
					//then subtract from all palette indices > colorIndex2. Then we can
					//shift over all the palette colors. Then regenerate the palette.
					for (int i = 0; i < tilesX * tilesY; i++) {
						if (tileData[i].paletteIndex == colorIndex2 / 2) {
							tileData[i].paletteIndex = colorIndex1 / 2;
						} else if (tileData[i].paletteIndex > colorIndex2 / 2) {
							tileData[i].paletteIndex -= nColorsInPalettes / 2;
						}
					}

					//move entries in palette and colorTable.
					memmove(palette + colorIndex2, palette + colorIndex2 + nColorsInPalettes, (nPalettes * 2 - colorIndex2 - nColorsInPalettes) * sizeof(COLOR));
					memmove(colorTable + colorIndex2, colorTable + colorIndex2 + nColorsInPalettes, nPalettes * 2 - colorIndex2 - nColorsInPalettes);

					//merge those palettes that we've just combined.
					mergePalettes(tileData, tilesX * tilesY, palette, colorIndex1 / 2, palettesMode);

					//update end pointer to reflect the change.
					firstSlot -= nColorsInPalettes;
				}

				//now add this tile's colors
				if (!fits) {
					memcpy(palette + firstSlot, tile->palette, nConsumed * sizeof(COLOR));
					BYTE fill = 1 << (tile->mode >> 14);
					memset(colorTable + firstSlot, fill, nConsumed);
					tile->paletteIndex = firstSlot / 2;
					firstSlot += nConsumed;
				}
			}
			_globColors++;
		}
	}
	free(colorTable);
	return firstSlot;
}

void expandPalette(COLOR *nnsPal, WORD mode, DWORD *dest, int *nOpaque) {
	dest[0] = ColorConvertFromDS(nnsPal[0]);
	dest[1] = ColorConvertFromDS(nnsPal[1]);
	mode &= 0xC000;
	if (mode == 0x8000 || mode == 0xC000) *nOpaque = 4;
	else *nOpaque = 3;
	
	if (mode == 0x8000) {
		dest[2] = ColorConvertFromDS(nnsPal[2]);
		dest[3] = ColorConvertFromDS(nnsPal[3]);
	} else if (mode == 0x0000) {
		dest[2] = ColorConvertFromDS(nnsPal[2]);
		dest[3] = 0;
	} else if (mode == 0x4000) {
		dest[2] = blend(dest[0], 4, dest[1], 4);
		dest[3] = 0;
	} else if (mode == 0xC000) {
		dest[2] = blend(dest[0], 5, dest[1], 3);
		dest[3] = blend(dest[0], 3, dest[1], 5);
	}
}

WORD findOptimalPidx(DWORD *px, int hasTransparent, COLOR *palette, int nColors) {
	//yes, iterate over every possible palette and mode.
	int leastError = 0x10000000;
	WORD leastPidx = 0;
	for (int i = 0; i < nColors; i += 2) {
		COLOR *thisPalette = palette + i;
		DWORD expand[4];
		
		for (int j = 0; j < 4; j++) {
			int nConsumed = 2;
			if (j == 0 || j == 2) nConsumed = 4;
			if (i + nConsumed > nColors) continue;
			
			WORD mode = j << 14;
			expandPalette(thisPalette, mode, expand, &nConsumed);
			if (hasTransparent && nConsumed == 4) continue;

			int dst = computeLMS(px, expand, nConsumed == 3);
			if (dst < leastError) {
				leastPidx = mode | (i / 2);
				leastError = dst;
			}
		}
	}
	return leastPidx;
}

int convert4x4(CREATEPARAMS *params) {
	//3-stage compression. First stage builds tile data, second stage builds palettes, third stage builds the final texture.
	if (params->colorEntries < 16) params->colorEntries = 16;
	params->colorEntries = (params->colorEntries + 7) & 0xFFFFFFF8;
	int width = params->width, height = params->height;
	int tilesX = width / 4, tilesY = height / 4;
	_globFinal = tilesX * tilesY * 3;
	_globColors = 0;
	TILEDATA *tileData = createTileData(params->px, tilesX, tilesY);

	//build the palettes.
	COLOR *nnsPal = (COLOR *) calloc(params->colorEntries, sizeof(COLOR));
	int nUsedColors;
	if (!params->useFixedPalette) {
		nUsedColors = buildPalette(nnsPal, params->colorEntries / 2, tileData, tilesX, tilesY, params->threshold);
	} else {
		nUsedColors = params->colorEntries;
		memcpy(nnsPal, params->fixedPalette, params->colorEntries * 2);
		_globColors += tilesX * tilesY;
	}
	if (nUsedColors & 7) nUsedColors += 8 - (nUsedColors & 7);
	if (nUsedColors < 16) nUsedColors = 16;

	//allocate index data.
	WORD *pidx = (WORD *) calloc(tilesX * tilesY, 2);

	//generate texel data.
	DWORD *txel = (DWORD *) calloc(tilesX * tilesY, 4);
	for (int i = 0; i < tilesX * tilesY; i++) {
		DWORD texel = 0;

		//double check that these settings are the most optimal for this tile.
		WORD idx = findOptimalPidx((DWORD *) tileData[i].rgb, tileData[i].transparentPixels, nnsPal, nUsedColors);
		WORD mode = idx & 0xC000;
		WORD index = idx & 0x3FFF;
		COLOR *thisPalette = nnsPal + (index * 2);
		pidx[i] = idx;

		DWORD palette[4];
		int paletteSize;
		expandPalette(thisPalette, mode, palette, &paletteSize);

		for (int j = 0; j < 16; j++) {
			int index = 0;
			DWORD col = ((DWORD *) tileData[i].rgb)[j];
			if ((col >> 24) < 0x80) {
				index = 3;
			} else {
				index = closestpalette(*(RGB *) &col, (RGB *) palette, paletteSize, NULL);
			}
			texel |= index << (j * 2);
		}
		txel[i] = texel;
		_globColors++;
	}

	//set fields in the texture
	params->dest->palette.nColors = nUsedColors;
	if (params->dest->palette.pal) free(params->dest->palette.pal);
	params->dest->palette.pal = nnsPal;
	memcpy(params->dest->palette.name, params->pnam, 16);
	if (params->dest->texels.cmp) free(params->dest->texels.cmp);
	if (params->dest->texels.texel) free(params->dest->texels.texel);
	params->dest->texels.cmp = (short *) pidx;
	params->dest->texels.texel = (char *) txel;
	params->dest->texels.texImageParam = (ilog2(width >> 3) << 20) | (ilog2(height >> 3) << 23) | (params->fmt << 26);
	
	free(tileData);
	return 0;
}

DWORD startConvert(void* lpParam) {
	CREATEPARAMS *params = (CREATEPARAMS *) lpParam;
	//begin conversion.
	switch (params->fmt) {
		case CT_DIRECT:
			convertDirect(params);
			break;
		case CT_4COLOR:
		case CT_16COLOR:
		case CT_256COLOR:
			convertPalette(params);
			break;
		case CT_A3I5:
		case CT_A5I3:
			convertTranslucent(params);
			break;
		case CT_4x4:
			convert4x4(params);
			break;
	}
	convertTexture(params->px, &params->dest->texels, &params->dest->palette, 0);
	//convertTexture outputs red and blue in the opposite order, so flip them here.
	for (int i = 0; i < params->width * params->height; i++) {
		DWORD p = params->px[i];
		params->px[i] = REVERSE(p);
	}
	_globFinished = 1;
	if(params->callback) params->callback(params->callbackParam);
	if (params->useFixedPalette) free(params->fixedPalette);
	return 0;
}
