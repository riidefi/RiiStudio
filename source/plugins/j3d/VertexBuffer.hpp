#pragma once

#include <common.h>
#include <vector>
#include <plugins/gc/GX/VertexTypes.hpp>
#include <oishii/v2/writer/binary_writer.hxx>

namespace riistudio::j3d {


struct VQuantization
{
	libcube::gx::VertexComponentCount comp = libcube::gx::VertexComponentCount(libcube::gx::VertexComponentCount::Position::xyz);
	libcube::gx::VertexBufferType type = libcube::gx::VertexBufferType(libcube::gx::VertexBufferType::Generic::f32);
	u8 divisor = 0;
	u8 bad_divisor = 0; //!< Accommodation for a bug on N's part
	u8 stride = 12;

	VQuantization(libcube::gx::VertexComponentCount c, libcube::gx::VertexBufferType t, u8 d, u8 bad_d, u8 s)
		: comp(c), type(t), divisor(d), bad_divisor(bad_d), stride(s)
	{}
	VQuantization(const VQuantization& other)
		: comp(other.comp), type(other.type), divisor(other.divisor), stride(other.stride)
	{}
	VQuantization() = default;
};

enum class VBufferKind
{
	position,
	normal,
	color,
	textureCoordinate,

	undefined = -1
};


template<typename TB, VBufferKind kind>
struct VertexBuffer
{
	VQuantization mQuant;
	std::vector<TB> mData;

	int ComputeComponentCount() const
	{
		switch (kind)
		{
		case VBufferKind::position:
			if (mQuant.comp.position == libcube::gx::VertexComponentCount::Position::xy)
				throw "Buffer: XY Pos Component count.";
			return static_cast<int>(mQuant.comp.position) + 2; // xy -> 2; xyz -> 3
		case VBufferKind::normal:
			return 3;
		case VBufferKind::color:
			return static_cast<int>(mQuant.comp.color) + 3; // rgb -> 3; rgba -> 4
		case VBufferKind::textureCoordinate:
			return static_cast<int>(mQuant.comp.texcoord) + 1; // s -> 1, st -> 2
		default: // never reached
			return 0;
		}
	}

	template<int n, typename T, glm::qualifier q>
	void readBufferEntryGeneric(oishii::BinaryReader& reader, glm::vec<n, T, q>& result)
	{
		for (int i = 0; i < ComputeComponentCount(); ++i)
		{
			switch (mQuant.type.generic)
			{
			case libcube::gx::VertexBufferType::Generic::u8:
				result[i] = reader.read<u8>() / f32(1 << mQuant.divisor);
				break;
			case libcube::gx::VertexBufferType::Generic::s8:
				result[i] = reader.read<s8>() / f32(1 << mQuant.divisor);
				break;
			case libcube::gx::VertexBufferType::Generic::u16:
				result[i] = reader.read<u16>() / f32(1 << mQuant.divisor);
				break;
			case libcube::gx::VertexBufferType::Generic::s16:
				result[i] = reader.read<s16>() / f32(1 << mQuant.divisor);
				break;
			case libcube::gx::VertexBufferType::Generic::f32:
				result[i] = reader.read<f32>();
				break;
			default:
				throw "Invalid buffer type!";
			}
		}
	}
	void readBufferEntryColor(oishii::BinaryReader& reader, libcube::gx::Color& result)
	{
		switch (mQuant.type.color)
		{
		case libcube::gx::VertexBufferType::Color::rgb565:
		{
			const u16 c = reader.read<u16>();
			result.r = (c & 0xF800) >> 11;
			result.g = (c & 0x07E0) >> 5;
			result.b = (c & 0x001F);
			break;
		}
		case libcube::gx::VertexBufferType::Color::rgb8:
			result.r = reader.read<u8>();
			result.g = reader.read<u8>();
			result.b = reader.read<u8>();
			break;
		case libcube::gx::VertexBufferType::Color::rgbx8:
			result.r = reader.read<u8>();
			result.g = reader.read<u8>();
			result.b = reader.read<u8>();
			reader.seek(1);
			break;
		case libcube::gx::VertexBufferType::Color::rgba4:
		{
			const u16 c = reader.read<u16>();
			result.r = (c & 0xF000) >> 12;
			result.g = (c & 0x0F00) >> 8;
			result.b = (c & 0x00F0) >> 4;
			result.a = (c & 0x000F);
			break;
		}
		case libcube::gx::VertexBufferType::Color::rgba6:
		{
			const u32 c = reader.read<u32>();
			result.r = (c & 0xFC0000) >> 18;
			result.g = (c & 0x03F000) >> 12;
			result.b = (c & 0x000FC0) >> 6;
			result.a = (c & 0x00003F);
			break;
		}
		case libcube::gx::VertexBufferType::Color::rgba8:
			result.r = reader.read<u8>();
			result.g = reader.read<u8>();
			result.b = reader.read<u8>();
			result.a = reader.read<u8>();
			break;
		default:
			throw "Invalid buffer type!";
		};
	}

	template<int n, typename T, glm::qualifier q>
	void writeBufferEntryGeneric(oishii::v2::Writer& writer, const glm::vec<n, T, q>& v) const
	{
		const auto cnt = ComputeComponentCount();
		for (int i = 0; i < cnt; ++i)
		{
			switch (mQuant.type.generic)
			{
			case libcube::gx::VertexBufferType::Generic::u8:
				writer.write<u8>(roundf(v[i] * (1 << mQuant.divisor)));
				break;
			case libcube::gx::VertexBufferType::Generic::s8:
				writer.write<s8>(roundf(v[i] * (1 << mQuant.divisor)));
				break;
			case libcube::gx::VertexBufferType::Generic::u16:
				writer.write<u16>(roundf(v[i] * (1 << mQuant.divisor)));
				break;
			case libcube::gx::VertexBufferType::Generic::s16:
				writer.write<s16>(roundf(v[i] * (1 << mQuant.divisor)));
				break;
			case libcube::gx::VertexBufferType::Generic::f32:
				writer.write<f32>(v[i]);
				break;
			default:
				throw "Invalid buffer type!";
			}
		}
	}
	void writeBufferEntryColor(oishii::v2::Writer& writer, const libcube::gx::Color& c) const
	{
		switch (mQuant.type.color)
		{
		case libcube::gx::VertexBufferType::Color::rgb565:
			writer.write<u16>(((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) | ((c.b & 0xf8) >> 3));
			break;
		case libcube::gx::VertexBufferType::Color::rgb8:
			writer.write<u8>(c.r);
			writer.write<u8>(c.g);
			writer.write<u8>(c.b);
			break;
		case libcube::gx::VertexBufferType::Color::rgbx8:
			writer.write<u8>(c.r);
			writer.write<u8>(c.g);
			writer.write<u8>(c.b);
			writer.write<u8>(0);
			break;
		case libcube::gx::VertexBufferType::Color::rgba4:
			writer.write<u16>(((c.r & 0xf0) << 8) | ((c.g & 0xf0) << 4) | (c.b & 0xf0) | ((c.a & 0xf0) >> 4));
			break;
		case libcube::gx::VertexBufferType::Color::rgba6:
		{
			// TODO: Verify
			u32 v = ((c.r & 0x3f) << 18) | ((c.g & 0x3f) << 12) | (c.b & 0x3f << 6) | (c.a & 0x3f);
			writer.write<u8>((v & 0x00ff0000) >> 16);
			writer.write<u8>((v & 0x0000ff00) >> 8);
			writer.write<u8>((v & 0x000000ff));
			break;
		}
		case libcube::gx::VertexBufferType::Color::rgba8:
			writer.write<u8>(c.r);
			writer.write<u8>(c.g);
			writer.write<u8>(c.b);
			writer.write<u8>(c.a);
			break;
		default:
			throw "Invalid buffer type!";
		};
	}

	// For dead cases
	template<int n, typename T, glm::qualifier q>
	void writeBufferEntryColor(oishii::v2::Writer& writer, const glm::vec<n, T, q>& v) const
	{
		assert(!"Invalid kind/type template match!");
	}
	void writeBufferEntryGeneric(oishii::v2::Writer& writer, const libcube::gx::Color& c) const
	{
		assert(!"Invalid kind/type template match!");
	}

	void writeData(oishii::v2::Writer& writer) const
	{
		switch (kind)
		{
		case VBufferKind::position:
			if (mQuant.comp.position == libcube::gx::VertexComponentCount::Position::xy)
				throw "Buffer: XY Pos Component count.";

			for (const auto& d : mData)
				writeBufferEntryGeneric(writer, d);
			break;
		case VBufferKind::normal:
			if (mQuant.comp.normal != libcube::gx::VertexComponentCount::Normal::xyz)
				throw "Buffer: NBT Vectors.";

			for (const auto& d : mData)
				writeBufferEntryGeneric(writer, d);
			break;
		case VBufferKind::color:
			for (const auto& d : mData)
				writeBufferEntryColor(writer, d);
			break;
		case VBufferKind::textureCoordinate:
			if (mQuant.comp.texcoord == libcube::gx::VertexComponentCount::TextureCoordinate::s)
				throw "Buffer: Single component texcoord vectors.";

			for (const auto& d : mData)
				writeBufferEntryGeneric(writer, d);
			break;
		}
	}

	VertexBuffer() {}
	VertexBuffer(VQuantization q)
		: mQuant(q)
	{}

};


}
