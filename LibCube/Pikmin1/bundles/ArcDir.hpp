#pragma once

#include "../allincludes.hpp"
#include <vector>
#include "../DCK/DCK.hpp"

// Archive half of the file
struct Arc
{
	Arc() = default;
	~Arc() = default;

	std::vector<u8> m_data;
};

// Entry, Dir file has many of these
struct Entry
{
	Entry() = default;
	~Entry() = default;

	u32 m_offset;
	u32 m_size;
	std::string m_path;
};

struct Dir
{
	constexpr static const char name[] = "Bundled Archive (ARC + DIR)";
	Dir() = default;
	~Dir() = default;

	u32 m_sizeOfFile;
	std::vector<Entry> m_entries;

	static void onRead(oishii::BinaryReader& bReader, Dir& context);
};