#pragma once

#include <LibRiiEditor/common.hpp>
#include <vector>
#include <array>

namespace libcube { namespace gx {

enum class TevColorArg
{
	cprev,
	aprev,
	c0,
	a0,
	c1,
	a1,
	c2,
	a2,
	texc,
	texa,
	rasc,
	rasa,
	one,
	half,
	konst,
	zero
};

enum class TevAlphaArg
{
	aprev,
	a0,
	a1,
	a2,
	texa,
	rasa,
	konst,
	zero
};

enum class TevBias
{
	zero,		//!< As-is
	add_half,	//!< Add middle gray
	sub_half	//!< Subtract middle gray
};

enum class TevReg
{
	prev,
	reg0,
	reg1,
	reg2,

	reg3 = prev
};

enum class TevColorOp
{
	add,
	subtract,

	comp_r8_gt = 8,
	comp_r8_eq,
	comp_gr16_gt,
	comp_gr16_eq,
	comp_bgr24_gt,
	comp_bgr24_eq,
	comp_rgb8_gt,
	comp_rgb8_eq
};

enum class TevScale
{
	scale_1,
	scale_2,
	scale_4,
	divide_2
};

enum class TevAlphaOp
{
	add,
	subtract,

	comp_r8_gt = 8,
	comp_r8_eq,
	comp_gr16_gt,
	comp_gr16_eq,
	comp_bgr24_gt,
	comp_bgr24_eq,
	// Different from ColorOp
	comp_a8_gt,
	comp_a8_eq
};


enum class ColorComponent
{
	r = 0,
	g,
	b,
	a
};


enum class TevKColorSel
{
	const_8_8,
	const_7_8,
	const_6_8,
	const_5_8,
	const_4_8,
	const_3_8,
	const_2_8,
	const_1_8,

	k0 = 12,
	k1,
	k2,
	k3,
	k0_r,
	k1_r,
	k2_r,
	k3_r,
	k0_g,
	k1_g,
	k2_g,
	k3_g,
	k0_b,
	k1_b,
	k2_b,
	k3_b,
	k0_a,
	k1_a,
	k2_a,
	k3_a
};


enum class TevKAlphaSel
{
	const_8_8,
	const_7_8,
	const_6_8,
	const_5_8,
	const_4_8,
	const_3_8,
	const_2_8,
	const_1_8,

	k0_r = 16,
	k1_r,
	k2_r,
	k3_r,
	k0_g,
	k1_g,
	k2_g,
	k3_g,
	k0_b,
	k1_b,
	k2_b,
	k3_b,
	k0_a,
	k1_a,
	k2_a,
	k3_a
};

enum class ColorSelChanLow
{
	color0a0,
	color1a1,

	ind_alpha = 5,
	normalized_ind_alpha, // ind_alpha in range [0, 255]
	null // zero
};
enum class ColorSelChanApi
{
	color0,
	color1,
	alpha0,
	alpha1,
	color0a0,
	color1a1,
	zero,

	ind_alpha,
	normalized_ind_alpha,
	null = 0xFF
};

enum class IndTexFormat
{
	_8bit,
	_5bit,
	_4bit,
	_3bit,
};

enum class IndTexBiasSel
{
	none,
	s,
	t,
	st,
	u,
	su,
	tu,
	stu
};
enum class IndTexAlphaSel
{
	off,
	s,
	t,
	u
};
enum class IndTexMtxID
{
	off,
	_0,
	_1,
	_2,

	s0 = 5,
	s1,
	s2,

	t0 = 9,
	t1,
	t2
};
enum class IndTexWrap
{
	off,
	_256,
	_128,
	_64,
	_32,
	_16,
	_0
};


struct TevStage
{
	// RAS1_TREF
	ColorSelChanApi rasOrder = ColorSelChanApi::null;
	u8 texMap, texCoord = 0;
	u8 rasSwap, texMapSwap = 0;
	
	struct ColorStage
	{
		// KSEL
		TevKColorSel constantSelection = TevKColorSel::k0;
		// COLOR_ENV
		TevColorArg a = TevColorArg::zero;
		TevColorArg b = TevColorArg::zero;
		TevColorArg c = TevColorArg::zero;
		TevColorArg d = TevColorArg::cprev;
		TevColorOp formula = TevColorOp::add;
		TevBias bias = TevBias::zero;
		TevScale scale = TevScale::scale_1;
		bool clamp = true;
		TevReg out = TevReg::prev;
	} colorStage;
	struct AlphaStage
	{
		TevAlphaArg a, b, c, d;
		TevAlphaOp formula;
		// KSEL
		TevKAlphaSel constantSelection;
		TevBias bias;
		TevScale scale;
		bool clamp;
		TevReg out;
	} alphaStage;
	struct IndirectStage
	{
		u8 indStageSel;
		IndTexFormat format;
		IndTexBiasSel bias;
		IndTexMtxID matrix;
		IndTexWrap wrapU, wrapV;
		bool addPrev;
		bool utcLod;
		IndTexAlphaSel alpha;
	} indirectStage;
};


struct Shader
{
// Fixed-size DL
	// SWAP table
	struct SwapTableEntry
	{
		ColorComponent r, g, b, a;
	};
	std::array<SwapTableEntry, 4> mSwapTable;

	struct IndOrder
	{
		u8 refMap, refCoord;
	};
	std::array<IndOrder, 4> mIndirectOrders;

// Variable-sized DL	
	std::vector<TevStage> mStages;
};

} } // namespace libcube::gx
