#pragma once

#include "binary_reader.hxx"

namespace oishii {

template <u32 m>
struct MagicInvalidity;

template <typename T, EndianSelect E>
inline T BinaryReader::endianDecode(T val) const noexcept
{
	if (!Options::MULTIENDIAN_SUPPORT)
		return val;

	bool be = false;

	switch (E)
	{
	case EndianSelect::Current:
		be = bigEndian;
		break;
	case EndianSelect::Big:
		be = true;
		break;
	case EndianSelect::Little:
		be = false;
		break;
	}

	if (!Options::PLATFORM_LE)
		be = !be;

	return be ? swapEndian<T>(val) : val;

}

template <typename T, EndianSelect E, bool unaligned>
T BinaryReader::peek()
{
	boundsCheck(sizeof(T));


#ifndef NDEBUG
	for (const auto& bp : mBreakPoints)
	{
		if (tell() >= bp.offset && tell() + sizeof(T) <= bp.offset + bp.size)
		{
			printf("Reading from %04u (0x%04x) sized %u\n", tell(), tell(), (u32)sizeof(T));
			warnAt("Breakpoint hit", tell(), tell() + sizeof(T));
			__debugbreak();
		}
	}
#endif

	if (!unaligned)
		alignmentCheck(sizeof(T));
	T decoded = endianDecode<T, E>(*reinterpret_cast<T*>(getStreamStart() + tell()));

	return decoded;
}
template <typename T, EndianSelect E, bool unaligned>
T BinaryReader::read()
{
	T decoded = peek<T, E, unaligned>();
	seek<Whence::Current>(sizeof(T));
	return decoded;
}


template <typename T, int num, EndianSelect E, bool unaligned>
std::array<T, num> BinaryReader::readX()
{
	std::array<T, num> result;

	for (auto& e : result)
		e = read<T, E, unaligned>();

	return result;
}

// TODO: Can rewrite read/peek to use peekAt
template <typename T, EndianSelect E, bool unaligned>
T BinaryReader::peekAt(int trans)
{
	if (!unaligned)
		boundsCheck(sizeof(T), tell() + trans);

#ifndef NDEBUG
	for (const auto& bp : mBreakPoints)
	{
		if (tell() + trans >= bp.offset && tell() + trans + sizeof(T) <= bp.offset + bp.size)
		{
			printf("Reading from %04u (0x%04x) sized %u\n", (u32)tell() + trans, (u32)tell() + trans, (u32)sizeof(T));
			warnAt("Breakpoint hit", tell() + trans, tell() + trans + sizeof(T));
			__debugbreak();
		}
	}
#endif
	T decoded = endianDecode<T, E>(*reinterpret_cast<T*>(getStreamStart() + tell() + trans));

	return decoded;
}
//template <typename T, EndianSelect E>
//T BinaryReader::readAt(int trans)
//{
//	T decoded = peekAt<T, E>(trans);
//	return decoded;
//}

template<typename THandler, typename Indirection, typename TContext, bool needsSeekBack>
inline void BinaryReader::invokeIndirection(TContext& ctx, u32 atPool)
{
	const u32 start = tell();
	const u32 ptr = Indirection::is_pointed ? (u32)read<typename Indirection::offset_t>() : 0;
	const u32 back = tell();

	seek<Indirection::whence>(ptr + Indirection::translation, atPool);
	if (Indirection::has_next)
	{
		invokeIndirection<THandler, typename Indirection::next, TContext, false>(ctx);
	}
	else
	{
		// FIXME: Update ScopedRegion to be usable here.
		u32 jump_save = 0;
		u32 jump_size_save = 0;

		enterRegion(THandler::name, jump_save, jump_size_save, start, sizeof(typename Indirection::offset_t)); // TODO -- ensure 0 for direct

		THandler::onRead(*this, ctx);

		exitRegion(jump_save, jump_size_save);
	}
	// With multiple levels of indirection, we only need to seek back on the outermost layer
	if (needsSeekBack)
		seek<Whence::Set>(back);
}

template <typename THandler,
	typename TIndirection,
	bool seekBack,
	typename TContext>
	void BinaryReader::dispatch(TContext& ctx, u32 atPool)
{
	invokeIndirection<THandler, TIndirection, TContext, seekBack>(ctx, atPool);
}
template<typename lastReadType, typename TInval>
void BinaryReader::signalInvalidityLast()
{
	// Unfortunate workaround, would prefer to do warn<type>
	TInval::warn(*this, sizeof(lastReadType));
}
template<typename lastReadType, typename TInval, int>
void BinaryReader::signalInvalidityLast(const char* msg)
{
	TInval::warn(*this, sizeof(lastReadType), msg);
}

template<u32 magic, bool critical>
inline void BinaryReader::expectMagic()
{
	if (read<u32, EndianSelect::Big>() != magic)
	{
		signalInvalidityLast<u32, MagicInvalidity<magic>>();

		if (critical)
			exit(1);
	}
}

} // namespace oishii
