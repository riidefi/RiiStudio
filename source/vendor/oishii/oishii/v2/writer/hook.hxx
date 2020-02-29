#pragma once

/*!
 * @file
 * @brief Defines the datablock hook structure.
 */

#include <string>

namespace oishii::v2 {

class Node;

struct Hook
{
	enum RelativePosition
	{
		Begin,
		End,
		EndOfChildren // End of this block + children
	};

	const Node* mBlock = nullptr; // Block referenced
	// Can also reference an ID, if Node is nullptr
	const std::string mId = "";

	RelativePosition mRelation;
	int mOffset; // Offset from relation


    inline Hook(const Node& block, RelativePosition relation = Begin, int offset = 0)
    : mBlock(&block), mRelation(relation), mOffset(offset)
    {}
	inline Hook(const std::string& id, RelativePosition relation = Begin, int offset = 0)
		: mId(id), mRelation(relation), mOffset(offset)
	{}
};

} // namespace DataBlock