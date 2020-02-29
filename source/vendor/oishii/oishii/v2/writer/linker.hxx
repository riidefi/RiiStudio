/*!
 * @file
 * @brief Headers for the DataBlock linker which coagulates all blocks, serializes and resolves links.
 */

#pragma once

#include <vector>
#include <string>
#include <memory>

#include "../../types.hxx"

#include "hook.hxx"
#include "node.hxx"

namespace oishii::v2 {

//class Node;
class Writer;

//! @brief Opaque helper class.
//!
class LinkerHelper;

//! @brief Gathers all nodes recursively, serializes to a file, and resolves links
//!
class Linker
{
	friend class LinkerHelper;
public:

	Linker() = default;
	~Linker() = default;

	//! @brief Gathers nodes recursively from the root into the layout.
	//!
	//! @param[in] root The root node.
	//!
	void gather(std::unique_ptr<Node> root, const std::string& nameSpace) noexcept;

	//! @brief Shuffle the layout.
	//!
	void shuffle();

	//! @brief Enforce linking restrictions that shuffling may have destroyed, reordering the layout as necessary.
	//!
	void enforceRestrictions();

	//! @brief Write the internal layout to a stream.
	//!
	//! @param[in] writer The output stream.
	//!
	void write(Writer& writer, bool shuffle = false);

	using PadFunction = void(*)(char* dst, u32 size);
	PadFunction mUserPad = nullptr;
	
private:
	struct LayoutElement
	{
		std::unique_ptr<Node> mNode;
		std::string mNamespace;

		LayoutElement(std::unique_ptr<Node> node, const std::string& Namespace)
			: mNode(std::move(node)), mNamespace(Namespace)
		{}
	};

	std::vector<LayoutElement> mLayout;

public:
	//! Associates namespaced IDs to writer positions.
	//!
	struct MapEntry
	{
		std::string symbol = "?";
		std::size_t begin = 0;
		std::size_t end = 0;

		LinkingRestriction restrict; //!< Only for external use
	};
	std::vector<MapEntry> mMap;
};

} // namespace oishii
