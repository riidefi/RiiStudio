/*!
 * @file
 * @brief Sources for the DataBlock linker.
 */

#include "linker.hxx"

#include "node.hxx"
#include "binary_writer.hxx"

#include <memory>
#include <string>

#ifndef assert
#include <cassert>
#endif

namespace oishii {

// Helpers
class LinkerHelper
{
public:
	static const Node& findNamespacedID(const Linker& linker, const std::string& symbol, const std::string& nameSpace, const std::string& blockName, std::string& resultName)
	{
		// TODO: This can be optimized, no need to check two constructed strings

		// On same level
		{
			const std::string nameSpacedSymbol = nameSpace.empty() ? symbol : nameSpace + "::" + symbol;
			for (const auto& entry : linker.mLayout)
				if ((entry.second.empty() ? entry.first->getId() : entry.second + "::" + entry.first->getId()) == nameSpacedSymbol)
				{
					//if (resultName)
						resultName = nameSpacedSymbol;
					return *entry.first;
				}
			}
		// Children
		{
			// TODO: NAME AND ID MUST MATCH. NOT INTENDED
			const std::string nameSpacePrefix = nameSpace.empty() ? "" : nameSpace + "::";
			const std::string nameSpacedSymbol = nameSpacePrefix + (blockName.empty() ? "" : blockName + "::") + symbol;
			for (const auto& entry : linker.mLayout)
				if ((entry.second.empty() ? entry.first->getId() : entry.second + "::" + entry.first->getId()) == nameSpacedSymbol)
				{
					//if (resultName)
						resultName = nameSpacedSymbol;
					return *entry.first;
				}
		}
		// Global
		{
			for (const auto& entry : linker.mLayout)
				if (entry.second == symbol)
				{
					//if (resultName)
						resultName = symbol;
					return *entry.first;
				}
		}
		assert(!"Failed critical namespaced symbol lookup in layout");
	}
	// TODO: Offset might be better removed
	static u32 resolveHook(const Linker& linker, const std::string& symbol, Hook::RelativePosition pos, int offset = 0)
	{
		std::string symbol_ = symbol;
		// TODO: This can be much more optimized
		if (pos == Hook::RelativePosition::EndOfChildren)
		{
			if (!symbol_.empty())
				symbol_ += "::";
			symbol_ += "EndOfChildren";
		}
		for (const auto& entry : linker.mMap)
		{
			if (entry.symbol == symbol_)
			{
				switch (pos)
				{
					// todo: EndOfChildren
				case Hook::RelativePosition::Begin:
				case Hook::RelativePosition::EndOfChildren: // begin of marker node
					return (u32)(entry.begin + offset);
				case Hook::RelativePosition::End:
					return (u32)(entry.end + offset);
				default:
					printf("Linker Error: Unknown hook type %u -- assuming Begin\n", pos);
					return (u32)(entry.begin + offset);
				}
			}
		}
		printf("Linker Error: Cannot resolve symbol \"%s\"!\n", symbol_.c_str());
		return 0xcccccccc;
	}
};

struct EndOfChildrenMarker : public Node
{
	const Node& mParent;
	EndOfChildrenMarker(const Node& parent) : mParent(parent), Node("EndOfChildren", { LinkingRestriction::Leaf })
	{
		bLinkerOwned = true;
	}
	~EndOfChildrenMarker() {}

	eResult write(Writer&) const noexcept override { return eResult::Success; }
};

Linker::Linker()
{}
Linker::~Linker()
{
	//	for (const auto entry : mLayout) if (entry.first)
	//		delete entry.first;
}


void Linker::gather(const Node& root, const std::string& nameSpace) noexcept
{
	// We call this recursively	
	
	// Add the node
	mLayout.push_back(LayoutElement(&root, nameSpace));
	// Gather children
	std::vector<const Node*> children;
	const auto result = root.getChildren(children);
	assert(result == Node::eResult::Success);
	// Then it's children
	for (const auto child : children)
	{
		gather(*child, (nameSpace.empty() ? "" : (nameSpace + "::")) + root.getId());
	}
	// TODO: Hacky..
	// Leaf nodes do not have EndOfChildren markers
	if(!(root.getLinkingRestriction().options & LinkingRestriction::Leaf))
		mLayout.push_back(
			LayoutElement(
				new EndOfChildrenMarker(root),
				(nameSpace.empty() ? "" : (nameSpace + "::")) + root.getId()
			)
		);
}

void Linker::shuffle()
{
	// TODO: Shuffle and fix
	// TODO: Namespace type + allow ID and name different lookup
}

void Linker::enforceRestrictions()
{

}

void Linker::write(Writer& writer, bool doShuffle)
{
	if (doShuffle)
	{
		shuffle();
		enforceRestrictions();
	}

	// Write data
	for (const auto& entry : mLayout)
	{
		// align
		u32 alignment = entry.first->getLinkingRestriction().alignment;
		if (alignment)
			while (writer.tell() % alignment)
				writer.seek<Whence::Current>(1);

		// Fill map: symbol and begin position
		mMap.push_back({ (entry.second.empty() ? "" : entry.second + "::") + entry.first->getId(), writer.tell(), 0, entry.first->getLinkingRestriction() });
		// Write
		writer.mNameSpace = entry.second;
		writer.mBlockName = entry.first->getId();
		entry.first->write(writer);
		// Set ending position
		mMap[mMap.size() - 1].end = writer.tell();
	}

	{
		printf("Begin    End      Size     Align    Static Leaf  Symbol\n");
		for (const auto& entry : mMap)
		{
			printf("0x%06x 0x%06x 0x%06x 0x%06x %s  %s %s\n",
				(u32)entry.begin, (u32)entry.end, (u32)entry.end - (u32)entry.begin,
				entry.restrict.alignment,
				entry.restrict.options & LinkingRestriction::Static ? "true " : "false",
				entry.restrict.options & LinkingRestriction::Leaf ? "true " : "false",
				entry.symbol.c_str());
		}
	}

	// Resolve

	// TODO: map::ktpt::...::enpt is map::enpt
	for (const auto& reserve : writer.mLinkReservations)
	{
		// todo: make map
		const u32 addr = reserve.addr;
		const Link& link = reserve.mLink;
		const std::string& nameSpace = reserve.nameSpace.empty() ? "" : reserve.nameSpace + "::";

	
		// Order: local -> children -> global

		std::string fromBlockSymbol;
		std::string toBlockSymbol;

		const Node& from = link.from.mBlock ? *link.from.mBlock : LinkerHelper::findNamespacedID(*this, link.from.mId, nameSpace, reserve.blockName, fromBlockSymbol);
		const Node& to = link.to.mBlock ? *link.to.mBlock : LinkerHelper::findNamespacedID(*this, link.to.mId, nameSpace, reserve.blockName, toBlockSymbol);

		// TODO: Generalize all of these from/to methods
		if (link.from.mBlock)
		{
			bool bSuccess = false;
			for (const auto& entry : mLayout)
			{
				if (entry.first == link.from.mBlock)
				{
					fromBlockSymbol = entry.second.empty() ? entry.first->getId() : entry.second + "::" + entry.first->getId();
					bSuccess = true;
					break;
				}
			}
			if (!bSuccess)
				printf("Linker Error: Block %s was never written to stream, so canot be resolved.\n", link.from.mBlock->getId().c_str());
		}
		if (link.to.mBlock)
		{
			bool bSuccess = false;
			for (const auto& entry : mLayout)
			{
				if (entry.first == link.to.mBlock)
				{
					toBlockSymbol = entry.second.empty() ? entry.first->getId() : entry.second + "::" + entry.first->getId();
					bSuccess = true;
					break;
				}
			}
			if (!bSuccess)
				printf("Linker Error: Block %s was never written to stream, so canot be resolved.\n", link.to.mBlock->getId().c_str());
		}
		// TODO: Link: EndOfChildren + put that in map + if not all children static and in shuffle, supply random number
		const u32 fromAddr = LinkerHelper::resolveHook(*this, fromBlockSymbol, link.from.mRelation, link.from.mOffset);
		const u32 toAddr = LinkerHelper::resolveHook(*this, toBlockSymbol, link.to.mRelation, link.to.mOffset);


		writer.seek<Whence::Set>(addr);

		writer.writeN(reserve.TSize, toAddr - fromAddr);

	}

	// Clear resources. We may want to move this to the destructor
	for (const auto& entry : mLayout)
	{
		if (entry.first && entry.first->isOwnedByLinker())
			delete entry.first;
	}
}

} // namespace DataBlock
