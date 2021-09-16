#include <stdio.h>
#include "texture.h"

#ifdef _WIN32
#include <Windows.h>
#else
typedef unsigned char BYTE;

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getTexelSize(int width, int height, int texImageParam) {
	int nPx = width * height;
	int bits[] = { 0, 8, 2, 4, 8, 2, 8, 16 };
	int b = bits[FORMAT(texImageParam)];
	return (nPx * b) >> 3;
}

typedef struct {
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
} RGB;

void getrgb(unsigned short n, RGB * ret){
	int r = n & 0x1F;
	n >>= 5;
	int g = n & 0x1F;
	n >>= 5;
	int b = n & 0x1F;
	n >>= 5;
	ret->r = (BYTE) (r * 255 / 31);
	ret->g = (BYTE) (g * 255 / 31);
	ret->b = (BYTE) (b * 255 / 31);
	ret->a = (BYTE) (255 * n);
}

int max16Len(char *str) {
	int len = 0;
	for (int i = 0; i < 16; i++) {
		char c = str[i];
		if (!c) return len;
		len++;
	}
	return len;
}

char *stringFromFormat(int fmt) {
	char *fmts[] = {"", "a3i5", "palette4", "palette16", "palette256", "tex4x4", "a5i3", "direct"};
	return fmts[fmt];
}

void convertTexture(uint32_t* px, TEXELS* texels, PALETTE* palette, int flip) {
	int format = FORMAT(texels->texImageParam);
	int c0xp = COL0TRANS(texels->texImageParam);
	int width = TEXW(texels->texImageParam);
	int height = TEXH(texels->texImageParam);
	int nPixels = width * height;
	int txelSize = getTexelSize(width, height, texels->texImageParam);
	switch (format) {
		case CT_DIRECT:
		{
			for(int i = 0; i < nPixels; i++){
				unsigned short pVal = *(((unsigned short *) texels->texel) + i);
				RGB rgb = {0};
				getrgb(pVal, &rgb);
				px[i] = rgb.b | (rgb.g << 8) | (rgb.r << 16) | (rgb.a << 24);
			}
			break;
		}
		case CT_4COLOR:
		{
			int offs = 0;
			for(int i = 0; i < txelSize >> 2; i++){
				unsigned d = (unsigned) *(((int *) texels->texel) + i);
				for(int j = 0; j < 16; j++){
					int pVal = d & 0x3;
					d >>= 2;
					if (pVal < palette->nColors) {
						unsigned short col = palette->pal[pVal] | 0x8000;
						if (!pVal && c0xp) col = 0;
						RGB rgb = { 0 };
						getrgb(col, &rgb);
						px[offs] = rgb.b | (rgb.g << 8) | (rgb.r << 16) | (rgb.a << 24);
					}
					offs++;
				}
			}
			break;
		}
		case CT_16COLOR:
		{
			int iters = txelSize;
			for(int i = 0; i < iters; i++){
				unsigned char pVal = *(((unsigned char *) texels->texel) + i);
				unsigned short col0 = 0;
				unsigned short col1 = 0;
				if ((pVal & 0xF) < palette->nColors) {
					col0 = palette->pal[pVal & 0xF] | 0x8000;
				}
				if ((pVal >> 4) < palette->nColors) {
					col1 = palette->pal[pVal >> 4] | 0x8000;
				}
				if(c0xp){
					if(!(pVal & 0xF)) col0 = 0;
					if(!(pVal >> 4)) col1 = 0;
				}
				RGB rgb = {0};
				getrgb(col0, &rgb);
				px[i * 2] = rgb.b | (rgb.g << 8) | (rgb.r << 16) | (rgb.a << 24);
				getrgb(col1, &rgb);
				px[i * 2 + 1] = rgb.b | (rgb.g << 8) | (rgb.r << 16) | (rgb.a << 24);
			}
			break;
		}
		case CT_256COLOR:
		{
			for(int i = 0; i < txelSize; i++){
				unsigned char pVal = *(texels->texel + i);
				if (pVal < palette->nColors) {
					unsigned short col = *(((unsigned short *) palette->pal) + pVal) | 0x8000;
					if (!pVal && c0xp) col = 0;
					RGB rgb = { 0 };
					getrgb(col, &rgb);
					px[i] = rgb.b | (rgb.g << 8) | (rgb.r << 16) | (rgb.a << 24);
				}
			}
			break;
		}
		case CT_A3I5:
		{
			for(int i = 0; i < txelSize; i++){
				unsigned char d = texels->texel[i];
				int alpha = ((d & 0xE0) >> 5) * 255 / 7;
				int index = d & 0x1F;
				if (index < palette->nColors) {
					unsigned short atIndex = *(((unsigned short *) palette->pal) + index);
					RGB r = { 0 };
					getrgb(atIndex, &r);
					r.a = alpha;
					px[i] = r.b | (r.g << 8) | (r.r << 16) | (r.a << 24);
				}
			}
			break;
		}
		case CT_A5I3:
		{
			for(int i = 0; i < txelSize; i++){
				unsigned char d = texels->texel[i];
				int alpha = ((d & 0xF8) >> 3) * 255 / 31;
				int index = d & 0x7;
				if (index < palette->nColors) {
					unsigned short atIndex = *(((unsigned short *) palette->pal) + index);
					RGB r = { 0 };
					getrgb(atIndex, &r);
					r.a = alpha;
					px[i] = r.b | (r.g << 8) | (r.r << 16) | (r.a << 24);
				}
			}
			break;
		}
		case CT_4x4:
		{
			int squares = (width * height) >> 4;
			RGB colors[4] = { 0 };
			RGB transparent = {0, 0, 0, 0};
			for(int i = 0; i < squares; i++){
				unsigned texel = *(unsigned *) (texels->texel + (i << 2));
				unsigned short data = *(unsigned short *) (texels->cmp + i);

				int address = (data & 0x3FFF) << 1;
				int mode = (data >> 14) & 0x3;
				if (address < palette->nColors) {
					unsigned short * base = ((unsigned short *) palette->pal) + address;
					getrgb(base[0], colors);
					getrgb(base[1], colors + 1);
					colors[0].a = 255;
					colors[1].a = 255;
					if (mode == 0) {
						getrgb(base[2], colors + 2);
						colors[2].a = 255;
						colors[3] = transparent;
					} else if (mode == 1) {
						RGB col0 = *colors;
						RGB col1 = *(colors + 1);
						colors[2].r = (col0.r + col1.r) >> 1;
						colors[2].g = (col0.g + col1.g) >> 1;
						colors[2].b = (col0.b + col1.b) >> 1;
						colors[2].a = 255;
						colors[3] = transparent;
					} else if (mode == 2) {
						getrgb(base[2], colors + 2);
						getrgb(base[3], colors + 3);
						colors[2].a = 255;
						colors[3].a = 255;
					} else {
						RGB col0 = *colors;
						RGB col1 = *(colors + 1);
						colors[2].r = (col0.r * 5 + col1.r * 3) >> 3;
						colors[2].g = (col0.g * 5 + col1.g * 3) >> 3;
						colors[2].b = (col0.b * 5 + col1.b * 3) >> 3;
						colors[2].a = 255;
						colors[3].r = (col0.r * 3 + col1.r * 5) >> 3;
						colors[3].g = (col0.g * 3 + col1.g * 5) >> 3;
						colors[3].b = (col0.b * 3 + col1.b * 5) >> 3;
						colors[3].a = 255;
					}
				}
				for(int j = 0; j < 16; j++){
					int pVal = texel & 0x3;
					texel >>= 2;
					RGB rgb = colors[pVal];
					int offs = ((i & ((width >> 2) - 1)) << 2) + (j & 3) + (((i / (width >> 2)) << 2) + (j  >> 2)) * width;
					px[offs] = rgb.b | (rgb.g << 8) | (rgb.r << 16) | (rgb.a << 24);
				}
			}
			break;
		}
	}
	//flip upside down
	if (flip) {
		uint32_t *tmp = calloc(width, 4);
		for (int y = 0; y < height / 2; y++) {
			uint32_t *row1 = px + y * width;
			uint32_t *row2 = px + (height - 1 - y) * width;
			memcpy(tmp, row1, width * 4);
			memcpy(row1, row2, width * 4);
			memcpy(row2, tmp, width * 4);
		}
		free(tmp);
	}
}

#ifdef _WIN32

#pragma comment(lib, "Version.lib")

void getVersion(char *buffer, int max) {
	WCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	DWORD handle;
	DWORD dwSize = GetFileVersionInfoSize(path, &handle);
	if (dwSize) {
		BYTE *buf = (BYTE *) calloc(1, dwSize);
		if (GetFileVersionInfo(path, handle, dwSize, buf)) {
			UINT size;
			VS_FIXEDFILEINFO *info;
			if (VerQueryValue(buf, L"\\", &info, &size)) {		
				DWORD ms = info->dwFileVersionMS, ls = info->dwFileVersionLS;
				sprintf(buffer, "%d.%d.%d.%d", HIWORD(ms), LOWORD(ms), HIWORD(ls), LOWORD(ls));
			}
		}
		free(buf);
	}
}

void writeNitroTGA(LPWSTR name, TEXELS *texels, PALETTE *palette) {
	HANDLE hFile = CreateFile(name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwWritten;

	int width = TEXW(texels->texImageParam);
	int height = TEXH(texels->texImageParam);
	DWORD *pixels = (DWORD *) calloc(width * height, 4);
	convertTexture(pixels, texels, palette, 1);

	BYTE header[] = {0x14, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20, 8,
		'N', 'N', 'S', '_', 'T', 'g', 'a', ' ', 'V', 'e', 'r', ' ', '1', '.', '0', 0, 0, 0, 0, 0};
	*(WORD *) (header + 0xC) = width;
	*(WORD *) (header + 0xE) = height;
	*(DWORD *) (header + 0x22) = sizeof(header) + width * height * 4;
	WriteFile(hFile, header, sizeof(header), &dwWritten, NULL);
	WriteFile(hFile, pixels, width * height * 4, &dwWritten, NULL);

	char *fstr = stringFromFormat(FORMAT(texels->texImageParam));
	WriteFile(hFile, "nns_frmt", 8, &dwWritten, NULL);
	int flen = strlen(fstr) + 0xC;
	WriteFile(hFile, &flen, 4, &dwWritten, NULL);
	WriteFile(hFile, fstr, flen - 0xC, &dwWritten, NULL);

	//texels
	WriteFile(hFile, "nns_txel", 8, &dwWritten, NULL);
	DWORD txelLength = getTexelSize(width, height, texels->texImageParam) + 0xC;
	WriteFile(hFile, &txelLength, 4, &dwWritten, NULL);
	WriteFile(hFile, texels->texel, txelLength - 0xC, &dwWritten, NULL);

	//write 4x4 if applicable
	if (FORMAT(texels->texImageParam) == CT_4x4) {
		WriteFile(hFile, "nns_pidx", 8, &dwWritten, NULL);
		DWORD pidxLength = (txelLength - 0xC) / 2 + 0xC;
		WriteFile(hFile, &pidxLength, 4, &dwWritten, NULL);
		WriteFile(hFile, texels->cmp, pidxLength - 0xC, &dwWritten, NULL);
	}

	//palette (if applicable)
	if (FORMAT(texels->texImageParam) != CT_DIRECT) {
		WriteFile(hFile, "nns_pnam", 8, &dwWritten, NULL);
		DWORD pnamLength = max16Len(palette->name) + 0xC;
		WriteFile(hFile, &pnamLength, 4, &dwWritten, NULL);
		WriteFile(hFile, palette->name, pnamLength - 0xC, &dwWritten, NULL);

		WriteFile(hFile, "nns_pcol", 8, &dwWritten, NULL);
		DWORD pcolLength = palette->nColors * 2 + 0xC;
		WriteFile(hFile, &pcolLength, 4, &dwWritten, NULL);
		WriteFile(hFile, palette->pal, palette->nColors * 2, &dwWritten, NULL);
	}

	BYTE gnam[] = {'n', 'n', 's', '_', 'g', 'n', 'a', 'm', 22, 0, 0, 0, 'N', 'i', 't', 'r', 'o', 'P', 'a', 'i', 'n', 't'};
	WriteFile(hFile, gnam, sizeof(gnam), &dwWritten, NULL);

	char version[16];
	getVersion(version, 16);
	BYTE gver[] = {'n', 'n', 's', '_', 'g', 'v', 'e', 'r', 0, 0, 0, 0};
	*(DWORD *) (gver + 8) = strlen(version) + 0xC;
	WriteFile(hFile, gver, sizeof(gver), &dwWritten, NULL);
	WriteFile(hFile, version, strlen(version), &dwWritten, NULL);

	BYTE imst[] = {'n', 'n', 's', '_', 'i', 'm', 's', 't', 0xC, 0, 0, 0};
	WriteFile(hFile, imst, sizeof(imst), &dwWritten, NULL);

	//if c0xp
	if (COL0TRANS(texels->texImageParam)) {
		BYTE c0xp[] = {'n', 'n', 's', '_', 'c', '0', 'x', 'p', 0xC, 0, 0, 0};
		WriteFile(hFile, c0xp, sizeof(c0xp), &dwWritten, NULL);
	}

	//write end
	BYTE end[] = {'n', 'n', 's', '_', 'e', 'n', 'd', 'b', 0xC, 0, 0, 0};
	WriteFile(hFile, end, sizeof(end), &dwWritten, NULL);

	CloseHandle(hFile);
	free(pixels);
}

int ilog2(int x) {
	int n = 0;
	while (x) {
		x >>= 1;
		n++;
	}
	return n - 1;
}

int textureDimensionIsValid(int x) {
	if (x & (x - 1)) return 0;
	if (x < 8 || x > 1024) return 0;
	return 1;
}

int nitrotgaIsValid(unsigned char *buffer, unsigned int size) {
	//is the file even big enough to hold a TGA header and comment?
	if (size < 0x16) return 0;
	
	int commentLength = *buffer;
	if (commentLength < 4) return 0;
	unsigned int ptrOffset = 0x12 + commentLength - 4;
	if (ptrOffset + 4 > size) return 0;
	unsigned int ptr = *(unsigned int *) (buffer + ptrOffset);
	if (ptr + 0xC > size) return 0;

	//process sections. When any anomalies are found, return 0.
	unsigned char *curr = buffer + ptr;
	while (1) {
		//is there space enough left for a section header?
		if (curr + 0xC > buffer + size) return 0;

		//all sections must start with nns_.
		if (*(int *) curr != 0x5F736E6E) return 0;
		int section = *(int *) (curr + 4);
		unsigned int sectionSize = *(unsigned int *) (curr + 8);

		//does the section size make sense?
		if (sectionSize < 0xC) return 0;

		//is the entire section within the file?
		if (curr - buffer + sectionSize > size) return 0;

		//Is this the end marker?
		if (section == 0x62646E65) return 1;
		curr += sectionSize;
	}

	return 1;
}

int nitroTgaRead(LPWSTR path, TEXELS *texels, PALETTE *palette) {
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwSizeHigh;
	DWORD dwSize = GetFileSize(hFile, &dwSizeHigh);
	LPBYTE lpBuffer = (LPBYTE) malloc(dwSize);
	DWORD dwRead;
	ReadFile(hFile, lpBuffer, dwSize, &dwRead, NULL);
	CloseHandle(hFile);

	if (!nitrotgaIsValid(lpBuffer, dwSize)) {
		free(lpBuffer);
		return 1;
	}

	int commentLength = *lpBuffer;
	int nitroOffset = *(int *) (lpBuffer + 0x12 + commentLength - 4);
	LPBYTE buffer = lpBuffer + nitroOffset;

	int width = *(short *) (lpBuffer + 0xC);
	int height = *(short *) (lpBuffer + 0xE);

	int frmt = 0;
	int c0xp = 0;
	char pnam[16] = { 0 };
	char *txel = NULL;
	char *pcol = NULL;
	char *pidx = NULL;

	int nColors = 0;

	while (1) {
		char sect[9] = { 0 };
		memcpy(sect, buffer, 8);
		if (!strcmp(sect, "nns_endb")) break;

		buffer += 8;
		int length = (*(int *) buffer) - 0xC;
		buffer += 4;

		if (!strcmp(sect, "nns_txel")) {
			txel = calloc(length, 1);
			memcpy(txel, buffer, length);
		} else if (!strcmp(sect, "nns_pcol")) {
			pcol = calloc(length, 1);
			memcpy(pcol, buffer, length);
			nColors = length / 2;
		} else if (!strcmp(sect, "nns_pidx")) {
			pidx = calloc(length, 1);
			memcpy(pidx, buffer, length);
		} else if (!strcmp(sect, "nns_frmt")) {
			if (!strncmp(buffer, "tex4x4", length)) {
				frmt = CT_4x4;
			} else if (!strncmp(buffer, "palette4", length)) {
				frmt = CT_4COLOR;
			} else if (!strncmp(buffer, "palette16", length)) {
				frmt = CT_16COLOR;
			} else if (!strncmp(buffer, "palette256", length)) {
				frmt = CT_256COLOR;
			} else if (!strncmp(buffer, "a3i5", length)) {
				frmt = CT_A3I5;
			} else if (!strncmp(buffer, "a5i3", length)) {
				frmt = CT_A5I3;
			} else if (!strncmp(buffer, "direct", length)) {
				frmt = CT_DIRECT;
			}
		} else if (!strcmp(sect, "nns_c0xp")) {
			c0xp = 1;
		} else if (!strcmp(sect, "nns_pnam")) {
			memcpy(pnam, buffer, length);
		}

		buffer += length;
	}

	if (frmt != CT_DIRECT) {
		palette->pal = (short *) pcol;
		palette->nColors = nColors;
		memcpy(palette->name, pnam, 16);
	}
	texels->cmp = (short *) pidx;
	texels->texel = txel;
	memcpy(texels->name, pnam, 16);
	int texImageParam = 0;
	if (c0xp) texImageParam |= (1 << 29);
	texImageParam |= (1 << 17) | (1 << 16);
	texImageParam |= (ilog2(width >> 3) << 20) | (ilog2(height >> 3) << 23);
	texImageParam |= frmt << 26;

	texels->texImageParam = texImageParam;

	return 0;
}

#endif
