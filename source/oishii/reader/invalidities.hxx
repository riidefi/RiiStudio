#pragma once

#include "binary_reader.hxx"

namespace oishii {


template<u32 m>
struct MagicInvalidity : public Invalidity
{
	enum
	{
		c0 = (m & 0xff000000) >> 24,
		c1 = (m & 0x00ff0000) >> 16,
		c2 = (m & 0x0000ff00) >> 8,
		c3 = (m & 0x000000ff),

		magic_big = MAKE_BE32(m)
	};

	static void warn(BinaryReader& reader, u32 sz)
	{
		char tmp[] = "File identification magic mismatch: expecting XXXX, instead saw YYYY.";
		*(u32*)&tmp[46] = magic_big;
		*(u32*)&tmp[64] = *(u32*)(reader.getStreamStart() + reader.tell() - 4);
		reader.warnAt(tmp, reader.tell() - sz, reader.tell());
	}
};
struct BOMInvalidity : public Invalidity
{
	static void warn(BinaryReader& reader, u32 sz)
	{
		char tmp[] = "File bye-order-marker (BOM) is invalid. Expected either 0xFFFE or 0xFEFF.";
		reader.warnAt(tmp, reader.tell() - sz, reader.tell());
	}
};

struct UncommonInvalidity : public Invalidity
{
	enum { inval_use_userdata_string };
	static void warn(BinaryReader& reader, u32 sz, const char* msg)
	{
		reader.warnAt(msg, reader.tell() - sz, reader.tell());
	}
};

} // namespace oishii
