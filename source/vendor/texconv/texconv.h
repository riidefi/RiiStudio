#pragma once

//
// Garhoogin TexConv
//

#include <stdint.h>

typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int BOOL;
typedef uint16_t WORD;

#include "texture.h"

typedef struct {
	DWORD *px;
	int width;
	int height;
	int fmt;
	int dither;
	int ditherAlpha;
	int colorEntries;
	BOOL useFixedPalette;
	COLOR *fixedPalette;
	int threshold;
	TEXTURE *dest;
	void (*callback) (void *);
	void *callbackParam;
	char pnam[17];
} CREATEPARAMS;

int countColors(DWORD *px, int nPx);

int convertDirect(CREATEPARAMS *params);

int convertPalette(CREATEPARAMS *params);

int convertTranslucent(CREATEPARAMS *params);

//progress markers for convert4x4.
extern volatile int _globColors;
extern volatile int _globFinal;
extern volatile int _globFinished;

int convert4x4(CREATEPARAMS *params);

//to convert a texture directly. lpParam is a CREATEPARAMS struct pointer.
DWORD startConvert(void* lpParam);

void threadedConvert(DWORD *px, int width, int height, int fmt, BOOL dither, BOOL ditherAlpha, int colorEntries, BOOL useFixedPalette, COLOR *fixedPalette, int threshold, char *pnam, TEXTURE *dest, void (*callback) (void *), void *callbackParam);
