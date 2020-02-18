#pragma once

#include "Enum/CullMode.hpp"
#include "Struct/ZMode.hpp"
#include "Struct/TexGen.hpp"
#include "Struct/Shader.hpp"
#include "Enum/Comparison.hpp"

namespace libcube { namespace gx {

enum class TextureWrapMode
{
	Clamp,
	Repeat,
	Mirror,
	MAX
};

enum class ColorSource
{
	Register,
	Vertex
};
enum class LightID
{
	None = 0,
	Light0 = 0x001,
	Light1 = 0x002,
	Light2 = 0x004,
	Light3 = 0x008,
	Light4 = 0x010,
	Light5 = 0x020,
	Light6 = 0x040,
	Light7 = 0x080
};
enum DiffuseFunction
{
	None,
	Sign,
	Clamp
};

enum class AttenuationFunction
{
	Specular,
	Spotlight,
	None,
	// Necessary for J3D compatibility at the moment.
	// Really we're looking at SpecDisabled/SpotDisabled there (2 adjacent bits in HW field)
	None2
};

struct ChannelControl
{
	bool enabled = false;
	ColorSource Ambient = ColorSource::Register;
	ColorSource Material = ColorSource::Register;
	LightID lightMask = LightID::None;
	DiffuseFunction diffuseFn = DiffuseFunction::None;
	AttenuationFunction attenuationFn = AttenuationFunction::None;

	bool operator==(const ChannelControl& other) const
	{
		return
			enabled == other.enabled &&
			Ambient == other.Ambient &&
			Material == other.Material &&
			lightMask == other.lightMask &&
			diffuseFn == other.diffuseFn &&
			attenuationFn == other.attenuationFn;
	}
};

enum class AlphaOp
{
	_and,
	_or,
	_xor,
	_xnor
};

struct AlphaComparison
{
	Comparison compLeft = Comparison::ALWAYS;
	u8 refLeft = 0;

	AlphaOp op = AlphaOp::_and;

	Comparison compRight = Comparison::ALWAYS;
	u8 refRight = 0;

	const bool operator==(const AlphaComparison& rhs) const noexcept
	{
		return compLeft == rhs.compLeft && refLeft == rhs.refLeft &&
			op == rhs.op &&
			compRight == rhs.compRight && refRight == rhs.refRight;
	}
};

enum class BlendModeType
{
	none,
	blend,
	logic,
	subtract
};
enum class BlendModeFactor
{
	zero,
	one,
	src_c,
	inv_src_c,
	src_a,
	inv_src_a,
	dst_a,
	inv_dst_a
};
enum class LogicOp
{
	_clear,
	_and,
	_rev_and,
	_copy,
	_inv_and,
	_no_op,
	_xor,
	_or,
	_nor,
	_equiv,
	_inv,
	_revor,
	_inv_copy,
	_inv_or,
	_nand,
	_set
};
// CMODE0
struct BlendMode
{
	BlendModeType type = BlendModeType::none;
	BlendModeFactor source = BlendModeFactor::src_a;
	BlendModeFactor dest = BlendModeFactor::inv_src_a;
	LogicOp logic = LogicOp::_clear;

	const bool operator==(const BlendMode& rhs) const noexcept
	{
		return type == rhs.type && source == rhs.source && dest == rhs.dest && logic == rhs.logic;
	}
};
struct Color;
struct ColorF32
{
	f32 r, g, b, a;

	inline operator float*()
	{
		return &r;
	}

	operator Color();
	inline bool operator==(const ColorF32& rhs) const
	{
		return (r == rhs.r) && (g == rhs.g) && (b == rhs.b) && (a == rhs.a);
	}
	inline bool operator!=(const ColorF32& rhs) const
	{
		return !operator==(rhs);
	}
};
struct Color
{
	u32 r, g, b, a;

	inline bool operator==(const Color& rhs) const noexcept
	{
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}

	inline operator ColorF32()
	{
		return {
			static_cast<float>(r) / static_cast<float>(0xff),
			static_cast<float>(g) / static_cast<float>(0xff),
			static_cast<float>(b) / static_cast<float>(0xff),
			static_cast<float>(a) / static_cast<float>(0xff)
		};
	}
	inline Color() : r(0), g(0), b(0), a(0) {}
	inline Color(u32 hex)
	{
		r = (hex & 0xff000000) << 24;
		g = (hex & 0x00ff0000) << 16;
		b = (hex & 0x0000ff00) << 8;
		a = (hex & 0x000000ff);
	}
	inline Color(u8 _r, u8 _g, u8 _b, u8 _a)
		: r(_r), g(_g), b(_b), a(_a)
	{}
};

inline ColorF32::operator Color()
{
	return {
		(u8)roundf(r * 255.0f),
		(u8)roundf(g * 255.0f),
		(u8)roundf(b * 255.0f),
		(u8)roundf(a * 255.0f)
	};
}
struct ColorS10
{
	s32 r, g, b, a;

	bool operator==(const ColorS10& rhs) const noexcept
	{
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}
	inline operator ColorF32()
	{
		return {
			static_cast<float>(r) / static_cast<float>(0xff),
			static_cast<float>(g) / static_cast<float>(0xff),
			static_cast<float>(b) / static_cast<float>(0xff),
			static_cast<float>(a) / static_cast<float>(0xff)
		};
	}
};


enum class AnisotropyLevel
{
	x1,
	x2,
	x4
};
enum class TextureFilter
{
	Near, // Nearest-neighbor interpolation of texels
	Linear, // Bi?Linear-neighbor interpolation of texels
	NearMipNear, // Nearest-neighbor interpolation of texels and mipmap levels
	LinearMipNear, // Bi?Linear interpolation of texels and nearest neighbor of mipmap levels
	NearMipLinear,
	LinearMipLinear, // Bi?Linear interpolation of texels and mipmap levels
	Max
};

struct ConstantAlpha
{
	bool enable = false;
	u8 value = 0;

	// void operator=(const GPU::CMODE1& reg);
};


} } // namespace libcube::gx
