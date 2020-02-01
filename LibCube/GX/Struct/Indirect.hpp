#pragma once

#include <LibCore/common.h>
#include <ThirdParty/glm/vec2.hpp>

namespace libcube { namespace gx {

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


struct IndirectTextureScalePair
{
	enum class Selection
	{
		x_1,	//!< 1     (2 ^  0) (<< 0)
		x_2,	//!< 1/2   (2 ^ -1) (<< 1)
		x_4,	//!< 1/4   (2 ^ -2) (<< 2)
		x_8,	//!< 1/8   (2 ^ -3) (<< 3)
		x_16,	//!< 1/16  (2 ^ -4) (<< 4)
		x_32,	//!< 1/32  (2 ^ -5) (<< 5)
		x_64,	//!< 1/64  (2 ^ -6) (<< 6)
		x_128,	//!< 1/128 (2 ^ -7) (<< 7)
		x_256	//!< 1/256 (2 ^ -8) (<< 8)
	};
	Selection U{ Selection::x_1 };
	Selection V{ Selection::x_1 };

	bool operator==(const IndirectTextureScalePair& rhs) const noexcept
	{
		return U == rhs.U && V == rhs.V;
	}
};
struct IndirectMatrix
{
	glm::vec2 scale { 0.5f, 0.5f };
	f32 rotate { 0.0f };
	glm::vec2 trans { 0.0f, 0.0f };

	int quant = 1; // Exponent selection

	bool operator==(const IndirectMatrix& rhs) const noexcept
	{
		return scale == rhs.scale && rotate == rhs.rotate && trans == rhs.trans && quant == rhs.quant;
	}
};
// The material part
struct IndirectSetting
{
	IndirectTextureScalePair texScale;
	IndirectMatrix mtx;

	IndirectSetting() {}
	IndirectSetting(const IndirectTextureScalePair& s, const IndirectMatrix& m)
		: texScale(s), mtx(m)
	{}

	bool operator==(const IndirectSetting& rhs) const noexcept
	{
		return texScale == rhs.texScale && mtx == rhs.mtx;
	}
};

// Used in TEV settings -- one per stage
struct IndOrder
{
	u8 refMap, refCoord;

	bool operator==(const IndOrder& rhs) const noexcept
	{
		return rhs.refMap == refMap && rhs.refCoord == refCoord;
	}
};
} }
