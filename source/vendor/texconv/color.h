#pragma once

typedef unsigned short COLOR;

#define COLOR_CHANNEL_R 0
#define COLOR_CHANNEL_G 1
#define COLOR_CHANNEL_B 2

#define GetR(c) ((c)&0x1F)
#define GetG(c) (((c)>>5)&0x1F)
#define GetB(c) (((c)>>10)&0x1F)
#define GetA(c) (((c)>>15)&1)

#define REVERSE(x) ((x)&0xFF00FF00)|(((x)&0xFF)<<16)|(((x)>>16)&0xFF)

#define CREVERSE(x) (((x)&0x83E0)|(((x)&0x1F)<<10)|(((x)>>10)&0x1F))

#define ColorSetChannel(c,ch,val) ((c)=(COLOR)(((c)&((0x1F<<((ch)*5))^0x7FFF))|(val)<<((ch)*5)))

#define ColorCreate(r,g,b) ((COLOR)((r)|((g)<<5)|((b)<<10)))

COLOR ColorConvertToDS(unsigned long c);

unsigned long ColorConvertFromDS(COLOR c);

COLOR ColorInterpolate(COLOR c1, COLOR c2, float amt);