/*
 * @file
 * @brief CMPR encoding. Based on WIMGT's implementation.
 */

#include <core/common.h>

#include <cstdlib>
#include <string.h>

#include <oishii/util/util.hxx>

namespace libcube {

const u8 cc58[32] = // convert 5-bit color to 8-bit color
{
	0x00,0x08,0x10,0x19, 0x21,0x29,0x31,0x3a, 0x42,0x4a,0x52,0x5a, 0x63,0x6b,0x73,0x7b,
	0x84,0x8c,0x94,0x9c, 0xa5,0xad,0xb5,0xbd, 0xc5,0xce,0xd6,0xde, 0xe6,0xef,0xf7,0xff
};

const u8 cc68[64] = // convert 6-bit color to 8-bit color
{
	0x00,0x04,0x08,0x0c, 0x10,0x14,0x18,0x1c, 0x20,0x24,0x28,0x2d, 0x31,0x35,0x39,0x3d,
	0x41,0x45,0x49,0x4d, 0x51,0x55,0x59,0x5d, 0x61,0x65,0x69,0x6d, 0x71,0x75,0x79,0x7d,
	0x82,0x86,0x8a,0x8e, 0x92,0x96,0x9a,0x9e, 0xa2,0xa6,0xaa,0xae, 0xb2,0xb6,0xba,0xbe,
	0xc2,0xc6,0xca,0xce, 0xd2,0xd7,0xdb,0xdf, 0xe3,0xe7,0xeb,0xef, 0xf3,0xf7,0xfb,0xff
};

const u8 cc85[256] = // convert 8-bit color to 5-bit color
{
	0x00,0x00,0x00,0x00, 0x00,0x01,0x01,0x01, 0x01,0x01,0x01,0x01, 0x01,0x02,0x02,0x02,
	0x02,0x02,0x02,0x02, 0x02,0x03,0x03,0x03, 0x03,0x03,0x03,0x03, 0x03,0x04,0x04,0x04,
	0x04,0x04,0x04,0x04, 0x04,0x04,0x05,0x05, 0x05,0x05,0x05,0x05, 0x05,0x05,0x06,0x06,
	0x06,0x06,0x06,0x06, 0x06,0x06,0x07,0x07, 0x07,0x07,0x07,0x07, 0x07,0x07,0x08,0x08,
	0x08,0x08,0x08,0x08, 0x08,0x08,0x09,0x09, 0x09,0x09,0x09,0x09, 0x09,0x09,0x09,0x0a,
	0x0a,0x0a,0x0a,0x0a, 0x0a,0x0a,0x0a,0x0b, 0x0b,0x0b,0x0b,0x0b, 0x0b,0x0b,0x0b,0x0c,
	0x0c,0x0c,0x0c,0x0c, 0x0c,0x0c,0x0c,0x0d, 0x0d,0x0d,0x0d,0x0d, 0x0d,0x0d,0x0d,0x0d,
	0x0e,0x0e,0x0e,0x0e, 0x0e,0x0e,0x0e,0x0e, 0x0f,0x0f,0x0f,0x0f, 0x0f,0x0f,0x0f,0x0f,
	0x10,0x10,0x10,0x10, 0x10,0x10,0x10,0x10, 0x11,0x11,0x11,0x11, 0x11,0x11,0x11,0x11,
	0x12,0x12,0x12,0x12, 0x12,0x12,0x12,0x12, 0x12,0x13,0x13,0x13, 0x13,0x13,0x13,0x13,
	0x13,0x14,0x14,0x14, 0x14,0x14,0x14,0x14, 0x14,0x15,0x15,0x15, 0x15,0x15,0x15,0x15,
	0x15,0x16,0x16,0x16, 0x16,0x16,0x16,0x16, 0x16,0x16,0x17,0x17, 0x17,0x17,0x17,0x17,
	0x17,0x17,0x18,0x18, 0x18,0x18,0x18,0x18, 0x18,0x18,0x19,0x19, 0x19,0x19,0x19,0x19,
	0x19,0x19,0x1a,0x1a, 0x1a,0x1a,0x1a,0x1a, 0x1a,0x1a,0x1b,0x1b, 0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1c, 0x1c,0x1c,0x1c,0x1c, 0x1c,0x1c,0x1c,0x1d, 0x1d,0x1d,0x1d,0x1d,
	0x1d,0x1d,0x1d,0x1e, 0x1e,0x1e,0x1e,0x1e, 0x1e,0x1e,0x1e,0x1f, 0x1f,0x1f,0x1f,0x1f
};

const u8 cc86[256] = // convert 8-bit color to 6-bit color
{
	0x00,0x00,0x00,0x01, 0x01,0x01,0x01,0x02, 0x02,0x02,0x02,0x03, 0x03,0x03,0x03,0x04,
	0x04,0x04,0x04,0x05, 0x05,0x05,0x05,0x06, 0x06,0x06,0x06,0x07, 0x07,0x07,0x07,0x08,
	0x08,0x08,0x08,0x09, 0x09,0x09,0x09,0x0a, 0x0a,0x0a,0x0a,0x0b, 0x0b,0x0b,0x0b,0x0c,
	0x0c,0x0c,0x0c,0x0d, 0x0d,0x0d,0x0d,0x0e, 0x0e,0x0e,0x0e,0x0f, 0x0f,0x0f,0x0f,0x10,
	0x10,0x10,0x10,0x11, 0x11,0x11,0x11,0x12, 0x12,0x12,0x12,0x13, 0x13,0x13,0x13,0x14,
	0x14,0x14,0x14,0x15, 0x15,0x15,0x15,0x15, 0x16,0x16,0x16,0x16, 0x17,0x17,0x17,0x17,
	0x18,0x18,0x18,0x18, 0x19,0x19,0x19,0x19, 0x1a,0x1a,0x1a,0x1a, 0x1b,0x1b,0x1b,0x1b,
	0x1c,0x1c,0x1c,0x1c, 0x1d,0x1d,0x1d,0x1d, 0x1e,0x1e,0x1e,0x1e, 0x1f,0x1f,0x1f,0x1f,
	0x20,0x20,0x20,0x20, 0x21,0x21,0x21,0x21, 0x22,0x22,0x22,0x22, 0x23,0x23,0x23,0x23,
	0x24,0x24,0x24,0x24, 0x25,0x25,0x25,0x25, 0x26,0x26,0x26,0x26, 0x27,0x27,0x27,0x27,
	0x28,0x28,0x28,0x28, 0x29,0x29,0x29,0x29, 0x2a,0x2a,0x2a,0x2a, 0x2a,0x2b,0x2b,0x2b,
	0x2b,0x2c,0x2c,0x2c, 0x2c,0x2d,0x2d,0x2d, 0x2d,0x2e,0x2e,0x2e, 0x2e,0x2f,0x2f,0x2f,
	0x2f,0x30,0x30,0x30, 0x30,0x31,0x31,0x31, 0x31,0x32,0x32,0x32, 0x32,0x33,0x33,0x33,
	0x33,0x34,0x34,0x34, 0x34,0x35,0x35,0x35, 0x35,0x36,0x36,0x36, 0x36,0x37,0x37,0x37,
	0x37,0x38,0x38,0x38, 0x38,0x39,0x39,0x39, 0x39,0x3a,0x3a,0x3a, 0x3a,0x3b,0x3b,0x3b,
	0x3b,0x3c,0x3c,0x3c, 0x3c,0x3d,0x3d,0x3d, 0x3d,0x3e,0x3e,0x3e, 0x3e,0x3f,0x3f,0x3f
};

extern const u8 cc85[256];	// convert 8-bit color to 5-bit color
extern const u8 cc86[256];	// convert 8-bit color to 6-bit color

enum {
	CMPR_MAX_COL = 16,
	CMPR_DATA_SIZE = 4 * CMPR_MAX_COL
};

inline void write_be16(void* dest, u16 val) {
	*reinterpret_cast<u16*>(dest) = oishii::swap16(val);
}


struct cmpr_info_t
{
	u32 opaque_count;
	u8		p[4][4];
	// 4 calculated values
	// p[0] + p[1]: values
	// if opaque_count < 16: p[2] == p[3]
};

static u32 calc_distance(const u8* v1, const u8* v2)
{
	const int d0 = (int)*v1++ - (int)*v2++;
	const int d1 = (int)*v1++ - (int)*v2++;
	const int d2 = (int)*v1++ - (int)*v2++;
	return abs(d0) + abs(d1) + abs(d2);
}

static u32 calc_distance_square(const u8* v1, const u8* v2)
{
	const int d0 = (int)*v1++ - (int)*v2++;
	const int d1 = (int)*v1++ - (int)*v2++;
	const int d2 = (int)*v1++ - (int)*v2++;
	return d0 * d0 + d1 * d1 + d2 * d2;
}

void CMPR_close_info(const u8* data,		// source data
	cmpr_info_t* info,		// info data structure
	u8* dest		// store destination data here (never 0)
)
{
	assert(info);
	assert(dest);

	u8* pal0 = info->p[0];
	u8* pal1 = info->p[1];

	if (info->opaque_count < CMPR_MAX_COL)
	{
		if (!info->opaque_count)
		{
			*dest++ = 0;
			*dest++ = 0;
			memset(dest, 0xff, 6);
			return;
		}

		// we have at least one transparent pixel

		u16 p0 = cc85[pal0[0]] << 11
			| cc86[pal0[1]] << 5
			| cc85[pal0[2]];
		u16 p1 = cc85[pal1[0]] << 11
			| cc86[pal1[1]] << 5
			| cc85[pal1[2]];
		if (p0 == p1)
		{
			// make p0 < p1
			p0 &= ~(u16)1;
			p1 |= 1;
		}
		else if (p0 > p1)
		{
			u16 ptemp = p0;
			p0 = p1;
			p1 = ptemp;
		}
		assert(p0 < p1);
		write_be16(dest, p0); dest += 2;
		write_be16(dest, p1); dest += 2;

		// re calculate palette colors

		pal0[0] = cc58[p0 >> 11];
		pal0[1] = cc68[p0 >> 5 & 0x3f];
		pal0[2] = cc58[p0 & 0x1f];
		pal0[3] = 0xff;

		pal1[0] = cc58[p1 >> 11];
		pal1[1] = cc68[p1 >> 5 & 0x3f];
		pal1[2] = cc58[p1 & 0x1f];
		pal1[3] = 0xff;

		// calculate median palette value

		u8* pal2 = info->p[2];
		pal2[0] = (pal0[0] + pal1[0]) / 2;
		pal2[1] = (pal0[1] + pal1[1]) / 2;
		pal2[2] = (pal0[2] + pal1[2]) / 2;
		pal2[3] = 0xff;
		memcpy(info->p[3], pal2, 4);

		for (u32 i = 0; i < 4; i++) {
			u8 val = 0;
			for (u32 j = 0; j < 4; j++, data += 4) {
				val <<= 2;
				if (data[3] & 0x80) {
					const u32 d0 = calc_distance(data, pal0);
					const u32 d1 = calc_distance(data, pal1);
					const u32 d2 = calc_distance(data, pal2);
					if (d1 <= d2)
						val |= d1 <= d0;
					else if (d2 < d0)
						val |= 2;
				}
				else {
					val |= 3;
				}
			}
			*dest++ = val;
		}
	} else {
		// we haven't any transparent pixel

		u16 p0 = cc85[pal0[0]] << 11
			| cc86[pal0[1]] << 5
			| cc85[pal0[2]];
		u16 p1 = cc85[pal1[0]] << 11
			| cc86[pal1[1]] << 5
			| cc85[pal1[2]];
		if (p0 == p1)
		{
			// make p0 > p1
			p0 |= 1;
			p1 &= ~(u16)1;
		}
		else if (p0 < p1)
		{
			u16 ptemp = p0;
			p0 = p1;
			p1 = ptemp;
		}
		assert(p0 > p1);
		write_be16(dest, p0); dest += 2;
		write_be16(dest, p1); dest += 2;

		// re calculate palette colors

		pal0[0] = cc58[p0 >> 11];
		pal0[1] = cc68[p0 >> 5 & 0x3f];
		pal0[2] = cc58[p0 & 0x1f];
		pal0[3] = 0xff;

		pal1[0] = cc58[p1 >> 11];
		pal1[1] = cc68[p1 >> 5 & 0x3f];
		pal1[2] = cc58[p1 & 0x1f];
		pal1[3] = 0xff;

		// calculate median palette values

		u8* pal2 = info->p[2];
		pal2[0] = (2 * pal0[0] + pal1[0]) / 3;
		pal2[1] = (2 * pal0[1] + pal1[1]) / 3;
		pal2[2] = (2 * pal0[2] + pal1[2]) / 3;
		pal2[3] = 0xff;

		u8* pal3 = info->p[3];
		pal3[0] = (pal0[0] + 2 * pal1[0]) / 3;
		pal3[1] = (pal0[1] + 2 * pal1[1]) / 3;
		pal3[2] = (pal0[2] + 2 * pal1[2]) / 3;
		pal3[3] = 0xff;

		u32 i;
		for (i = 0; i < 4; i++)
		{
			u8 val = 0;
			u32 j;
			for (j = 0; j < 4; j++, data += 4)
			{
				val <<= 2;
				const u32 d0 = calc_distance(data, pal0);
				const u32 d1 = calc_distance(data, pal1);
				const u32 d2 = calc_distance(data, pal2);
				const u32 d3 = calc_distance(data, pal3);
				if (d0 <= d1)
				{
					if (d2 <= d3)
						val |= d0 <= d2 ? 0 : 2;
					else
						val |= d0 <= d3 ? 0 : 3;
				}
				else
				{
					if (d2 <= d3)
						val |= d1 <= d2 ? 1 : 2;
					else
						val |= d1 <= d3 ? 1 : 3;
				}
			}
			*dest++ = val;
		}
	}

}


void WIMGT_CMPR(const u8* data, cmpr_info_t* info)
{
	assert(info);
	memset(info, 0, sizeof(*info));
	
	typedef struct sum_t
	{
		u8   col[4];
		u32 count;
	} sum_t;

	sum_t sum[CMPR_MAX_COL];
	u32 n_sum = 0, opaque_count = 0;

	const u8* data_end = data + CMPR_DATA_SIZE;
	const u8* dat;
	for (dat = data; dat < data_end; dat += 4)
	{
		if (dat[3] & 0x80)
		{
			opaque_count++;
			u8 col[4];
			col[0] = cc58[cc85[dat[0]]];
			col[1] = cc68[cc86[dat[1]]];
			col[2] = cc58[cc85[dat[2]]];

			for (u32 s = 0; s < n_sum; s++) {
				if (!memcmp(sum[s].col, col, 3)) {
					sum[s].count++;
					goto abort_s;
				}
			}
			col[3] = 0xff;
			memcpy(sum[n_sum].col, col, 4);
			sum[n_sum].count = 1;
			n_sum++;
		abort_s:;
		}
	}

	info->opaque_count = opaque_count;
	if (!opaque_count)
		return;

	assert(n_sum);
	if (n_sum < 3)
	{
		memcpy(info->p[0], sum[0].col, 4);
		memcpy(info->p[1], sum[n_sum - 1].col, 4);
		return;
	}

	assert(opaque_count >= 3);

	u32 best0 = 0, best1 = 0, max_dist = UINT_MAX;
	if (info->opaque_count < CMPR_MAX_COL)
	{
		// we have transparent points -> 1 middle point

		for (u32 s0 = 0; s0 < n_sum; s0++)
		{
			u8* pal0 = sum[s0].col;
			for (u32 s1 = s0 + 1; s1 < n_sum; s1++)
			{
				u8* pal1 = sum[s1].col;
				u8 pal2[4];
				pal2[0] = (pal0[0] + pal1[0]) / 2;
				pal2[1] = (pal0[1] + pal1[1]) / 2;
				pal2[2] = (pal0[2] + pal1[2]) / 2;

				u32 dist = 0;
				const u8* dat;
				for (dat = data; dat < data_end && dist < max_dist; dat += 4)
				{
					if (dat[3] & 0x80)
					{
						const u32 d0 = calc_distance(dat, pal0);
						const u32 d1 = calc_distance(dat, pal1);
						const u32 d2 = calc_distance(dat, pal2);
						if (d0 <= d1)
							dist += d0 < d2 ? d0 : d2;
						else
							dist += d1 < d2 ? d1 : d2;
					}
				}
				if (max_dist > dist)
				{
					max_dist = dist;
					best0 = s0;
					best1 = s1;
				}
			}
		}
	}
	else
	{
		// no transparent points -> 2 middle point

		u32 s0;
		for (s0 = 0; s0 < n_sum; s0++)
		{
			u8* pal0 = sum[s0].col;
			u32 s1;
			for (s1 = s0 + 1; s1 < n_sum; s1++)
			{
				u8* pal1 = sum[s1].col;
				u8 pal2[4];
				pal2[0] = (2 * pal0[0] + pal1[0]) / 3;
				pal2[1] = (2 * pal0[1] + pal1[1]) / 3;
				pal2[2] = (2 * pal0[2] + pal1[2]) / 3;
				u8 pal3[4];
				pal3[0] = (pal0[0] + 2 * pal1[0]) / 3;
				pal3[1] = (pal0[1] + 2 * pal1[1]) / 3;
				pal3[2] = (pal0[2] + 2 * pal1[2]) / 3;

				u32 dist = 0;
				const u8* dat;
				for (dat = data; dat < data_end && dist < max_dist; dat += 4)
				{
					const u32 d0 = calc_distance(dat, pal0);
					const u32 d1 = calc_distance(dat, pal1);
					const u32 d2 = calc_distance(dat, pal2);
					const u32 d3 = calc_distance(dat, pal3);
					if (d0 <= d1)
					{
						if (d2 <= d3)
							dist += d0 < d2 ? d0 : d2;
						else
							dist += d0 < d3 ? d0 : d3;
					}
					else
					{
						if (d2 <= d3)
							dist += d1 <= d2 ? d1 : d2;
						else
							dist += d1 <= d3 ? d1 : d3;
					}
				}
				if (max_dist > dist)
				{
					max_dist = dist;
					best0 = s0;
					best1 = s1;
				}
			}
		}
	}

	memcpy(info->p[0], sum[best0].col, 4);
	memcpy(info->p[1], sum[best1].col, 4);
}
struct Image_t;
u32 CalcImageSize
(
	u32		width,		// width of image in pixel
	u32		height,		// height of image in pixel

	u32		bits_per_pixel,	// number of bits per pixel
	u32		block_width,	// width of a single block
	u32		block_height,	// height of a single block

	u32* x_width,	// not NULL: store extended width here
	u32* x_height,	// not NULL: store extended height here
	u32* h_blocks,	// not NULL: store number of horizontal blocks
	u32* v_blocks	// not NULL: store number of vertical blocks
)
{
	const u32 hblocks = (width + block_width - 1) / block_width;
	if (h_blocks)
		*h_blocks = hblocks;

	const u32 xwidth = hblocks * block_width;
	if (x_width)
		*x_width = xwidth;

	const u32 vblocks = (height + block_height - 1) / block_height;
	if (v_blocks)
		*v_blocks = vblocks;

	const u32 xheight = vblocks * block_height;
	if (x_height)
		*x_height = xheight;

	return xwidth * xheight * bits_per_pixel / 8;
}

static void CalcImageBlock
(
	u32 width, u32 height,
	u32		bits_per_pixel,	// number of bits per pixel
	u32		block_width,	// width of a single block
	u32		block_height,	// height of a single block

	u32* h_blocks,	// not NULL: store number of horizontal blocks
	u32* v_blocks,	// not NULL: store number of vertical blocks
	u32* img_size	// not NULL: return image size
)
{
	assert(img);

	u32 xwidth, xheight;
	const u32 size = CalcImageSize(width, height,
		bits_per_pixel, block_width, block_height,
		&xwidth, &xheight, h_blocks, v_blocks);
	if (img_size)
		*img_size = size;
}

void EncodeDXT1(u8* dest_img, const u8* source_img, u32 width, u32 height)
{
	assert(dest_img);
	assert(source_img);
	
	const u32 bits_per_pixel = 4;
	const u32 block_width = 8;
	const u32 block_height = 8;

	u32 h_blocks, v_blocks, img_size;
	CalcImageBlock(width, height, bits_per_pixel, block_width, block_height,
		&h_blocks, &v_blocks, &img_size);

	u8* dest = dest_img;
	const u8* src1 = dest_img;

	const u32 block_size = block_width * 4;

	const u32 xwidth = (width + 0x1f) & ~0x1f;
	const u32 line_size = xwidth * 4;
	const u32 delta[] = { 0, 16, 4 * line_size, 4 * line_size + 16 };

	while (v_blocks-- > 0) {
		const u8* src2 = src1;
		u32 hblk = h_blocks;
		while (hblk-- > 0) {
			for (u32 subb = 0; subb < 4; subb++) {
				//---- first collect the data of the 16 pixel

				u8 vector[16 * 4], * vect = vector;
				const u8* src3 = src2 + delta[subb];
				for (u32 i = 0; i < 4; i++) {
					memcpy(vect, src3, 16);
					vect += 16;
					src3 += line_size;
				}
				assert(vect == vector + sizeof(vector));


				//--- analyze data

				cmpr_info_t info;
				WIMGT_CMPR(vector, &info);
				CMPR_close_info(vector, &info, dest);
				dest += 8;
			}
			src2 += block_size;
		}
		src1 += line_size * block_height;
	}

	assert(dest == data + img_size);
	assert(src1 <= dest_img->data + dest_img->data_size);
}


}
