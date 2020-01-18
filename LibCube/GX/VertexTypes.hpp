#pragma once

namespace libcube { namespace gx {

union VertexComponentCount
{
	
	enum class Position
	{
		xy,
		xyz
	} position;
	enum class Normal
	{
		xyz,
		nbt, // index NBT triplets
		nbt3 // index N/B/T individually
	} normal;
	enum class Color
	{
		rgb,
		rgba
	} color;
	enum class TextureCoordinate
	{
		s,
		st,

		u = s,
		uv = st
	} texcoord;

	explicit VertexComponentCount(Position p)
		: position(p)
	{}
	explicit VertexComponentCount(Normal n)
		: normal(n)
	{}
	explicit VertexComponentCount(Color c)
		: color(c)
	{}
	explicit VertexComponentCount(TextureCoordinate u)
		: texcoord(u)
	{}
	VertexComponentCount() {}
};

union VertexBufferType
{
	enum class Generic
	{
		u8,
		s8,
		u16,
		s16,
		f32
	} generic;
	enum class Color
	{
		rgb565, //!< R5G6B5
		rgb8, //!< 8 bit, no alpha
		rgbx8,  //!< 8 bit, alpha discarded
		rgba4, //!< 4 bit
		rgba6, //!< 6 bit
		rgba8, //!< 8 bit, alpha

		FORMAT_16B_565 = 0,
		FORMAT_24B_888 = 1,
		FORMAT_32B_888x = 2,
		FORMAT_16B_4444 = 3,
		FORMAT_24B_6666 = 4,
		FORMAT_32B_8888 = 5,
	} color;

	explicit VertexBufferType(Generic g)
		: generic(g)
	{}
	explicit VertexBufferType(Color c)
		: color(c)
	{}
	VertexBufferType() {}
};
enum class VertexAttribute
{
	PositionNormalMatrixIndex = 0,
	Texture0MatrixIndex,
	Texture1MatrixIndex,
	Texture2MatrixIndex,
	Texture3MatrixIndex,
	Texture4MatrixIndex,
	Texture5MatrixIndex,
	Texture6MatrixIndex,
	Texture7MatrixIndex,
	Position,
	Normal,
	Color0,
	Color1,
	TexCoord0,
	TexCoord1,
	TexCoord2,
	TexCoord3,
	TexCoord4,
	TexCoord5,
	TexCoord6,
	TexCoord7,

	PositionMatrixArray,
	NormalMatrixArray,
	TextureMatrixArray,
	LightArray,
	NormalBinormalTangent,
	Max,

	Undefined = 0xff - 1,
	Terminate = 0xff,

};
// Subset of vertex attributes valid for a buffer.
enum class VertexBufferAttribute : u32
{
	Position = 9,
	Normal,
	Color0,
	Color1,
	TexCoord0,
	TexCoord1,
	TexCoord2,
	TexCoord3,
	TexCoord4,
	TexCoord5,
	TexCoord6,
	TexCoord7,
	
	NormalBinormalTangent = 25,
	Max,

	Undefined = 0xff-1,
	Terminate = 0xff
};

enum class VertexAttributeType : u32
{
	None,	//!< No data is to be sent.
	Direct,	//!< Data will be sent directly.
	Byte,	//!< 8-bit indices.
	Short	//!< 16-bit indices.
};

enum class PrimitiveType
{
	Quads,          // 0x80
	Quads2,         // 0x88
	Triangles,      // 0x90
	TriangleStrip,  // 0x98
	TriangleFan,    // 0xA0
	Lines,          // 0xA8
	LineStrip,      // 0xB0
	Points,         // 0xB8
	Max
};
constexpr u32 PrimitiveMask = 0x78;
constexpr u32 PrimitiveShift = 3;

constexpr u32 EncodeDrawPrimitiveCommand(PrimitiveType type)
{
	return 0x80 | ((static_cast<u32>(type) << PrimitiveShift) & PrimitiveMask);
}
constexpr PrimitiveType DecodeDrawPrimitiveCommand(u32 cmd)
{
	return static_cast<PrimitiveType>((cmd & PrimitiveMask) >> PrimitiveShift);
}

static_assert(
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::Quads)) == 0x80 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::Quads2)) == 0x88 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::Triangles)) == 0x90 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::TriangleStrip)) == 0x98 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::TriangleFan)) == 0xA0 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::Lines)) == 0xA8 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::LineStrip)) == 0xB0 &&
	static_cast<u32>(EncodeDrawPrimitiveCommand(PrimitiveType::Points)) == 0xB8,
	"Primitive Command conversion failed");


} } // namespace libcube::gx
