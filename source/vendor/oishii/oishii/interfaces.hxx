#pragma once

#include "types.hxx"
#include <vector>
#include <cstdio>

namespace oishii {

enum class Whence
{
	Set, // Absolute -- start of file
	Current, // Relative -- current position
	End, // Relative -- end position
	// Only valid for invalidities so far
	Last, // Relative -- last read value

	At, // Indirection -- use specified translation (runtime) argument
};

class AbstractStream
{
public:
	virtual u32 tell() = 0;
	virtual void seekSet(u32 ofs) = 0;
	virtual u32 startpos() = 0;
	virtual u32 endpos() = 0;

	void breakPointProcess(u32 size)
	{
#ifndef NDEBUG
		for (const auto& bp : mBreakPoints)
		{
			if (tell() >= bp.offset && tell() + size <= bp.offset + bp.size)
			{
				printf("Writing to %04u (0x%04x) sized %u\n", tell(), tell(), size);
				// warnAt("Breakpoint hit", tell(), tell() + sizeof(T));
				__debugbreak();
			}
		}
#endif
	}

#ifndef NDEBUG
public:
	struct BP
	{
		u32 offset, size;
		BP(u32 o, u32 s)
			: offset(o), size(s)
		{}
	};
	void add_bp(u32 offset, u32 size)
	{
		mBreakPoints.emplace_back(offset, size);
	}
	template<typename T>
	void add_bp(u32 offset)
	{
		add_bp(offset, sizeof(T));
	}
	std::vector<BP> mBreakPoints;
#else
	void add_bp(u32, u32) {}
	template<typename T>
	void add_bp(u32) {}
#endif
};

class IReader : public AbstractStream
{
public:

	virtual unsigned char* getStreamStart() = 0;
};

class IWriter : public AbstractStream
{
};
} // namespace oishii
