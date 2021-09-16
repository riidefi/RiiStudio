#pragma once
#include <stdint.h>

typedef uint8_t BYTE;
typedef uint32_t DWORD;

DWORD getAverageColor(DWORD *colors, int nColors);

void getPrincipalComponent(DWORD *colors, int nColors, float *vec);

void getColorEndPoints(DWORD *colors, int nColors, DWORD *points);
