/*!
 * @file
 * @brief Sources for the DataBlock linker.
 */

#include "linker.hxx"

#include "binary_writer.hxx"
#include "node.hxx"

#include <algorithm>
#include <memory>
#include <string>

#include <core/common.h>

#ifndef assert
#include <cassert>
#endif

namespace oishii {

// Helpers
class LinkerHelper {
public:
  static const Node* findNamespacedID(const Linker& linker,
                                      const std::string& symbol,
                                      const std::string& nameSpace,
                                      const std::string& blockName,
                                      std::string& resultName) {
    // TODO: This can be optimized, no need to check two constructed strings

    // On same level
    {
      const std::string nameSpacedSymbol =
          nameSpace.empty() ? symbol : nameSpace + "::" + symbol;
      for (const auto& entry : linker.mLayout)
        if ((entry.mNamespace.empty()
                 ? entry.mNode->getId()
                 : entry.mNamespace + "::" + entry.mNode->getId()) ==
            nameSpacedSymbol) {
          // if (resultName)
          resultName = nameSpacedSymbol;
          return entry.mNode.get();
        }
    }
    // Children
    {
      std::string nameSpacePrefix = nameSpace.empty() ? "" : nameSpace + "::";
      //	if (hasEnding(nameSpacePrefix, "::::"))
      //		nameSpacePrefix = nameSpacePrefix.substr(0,
      // nameSpacePrefix.size() - 2);
      const std::string nameSpacedSymbol =
          nameSpacePrefix + (blockName.empty() ? "" : blockName + "::") +
          symbol;
      for (const auto& entry : linker.mLayout)
        if ((entry.mNamespace.empty()
                 ? entry.mNode->getId()
                 : entry.mNamespace + "::" + entry.mNode->getId()) ==
            nameSpacedSymbol) {
          // if (resultName)
          resultName = nameSpacedSymbol;
          return entry.mNode.get();
        }
    }
    // Global
    {
      for (const auto& entry : linker.mLayout)
        if ((entry.mNamespace.empty()
                 ? entry.mNode->getId()
                 : entry.mNamespace + "::" + entry.mNode->getId()) == symbol) {
          // if (resultName)
          resultName = symbol;
          return entry.mNode.get();
        }
    }
    printf("Search for %s failed!\n", symbol.c_str());
    assert(!"Failed critical namespaced symbol lookup in layout");
    return nullptr;
  }
  // TODO: Offset might be better removed
  static u32 resolveHook(const Linker& linker, const std::string& symbol,
                         Hook::RelativePosition pos, int offset = 0) {
    std::string symbol_ = symbol;
    // TODO: This can be much more optimized
    if (pos == Hook::RelativePosition::EndOfChildren) {
      if (!symbol_.empty())
        symbol_ += "::";
      symbol_ += "EndOfChildren";
    }
    for (const auto& entry : linker.mMap) {
      if (entry.symbol == symbol_) {
        switch (pos) {
        case Hook::RelativePosition::Begin:
        case Hook::RelativePosition::EndOfChildren: // begin of marker node
        {
          auto roundDown = [](u32 in, u32 align) -> u32 {
            return align ? in & ~(align - 1) : in;
          };
          auto roundUp = [roundDown](u32 in, u32 align) -> u32 {
            return align ? roundDown(in + (align - 1), align) : in;
          };
          u32 x = entry.begin + offset;
          // TODO: We don't need to make another pass from the start
          u32 align = pos == Hook::RelativePosition::Begin
                          ? entry.restrict.alignment
                          : std::find_if(linker.mMap.begin(), linker.mMap.end(),
                                         [symbol](auto& e) {
                                           return e.symbol == symbol;
                                         })
                                ->restrict.alignment;
          u32 rounded = roundUp(x, align);
          return rounded;
        }
        case Hook::RelativePosition::End:
          return entry.end + offset;
        default:
          printf("Linker Error: Unknown hook type %u -- assuming Begin (no "
                 "align)\n",
                 pos);
          return entry.begin + offset;
        }
      }
    }
    printf("Linker Error: Cannot resolve symbol \"%s\"!\n", symbol_.c_str());
    return 0xcccccccc;
  }
};

struct EndOfChildrenMarker : public Node {
  EndOfChildrenMarker(const Node& parent)
      : Node("EndOfChildren", {.Leaf = true}), mParent(parent) {}

  const Node& mParent;
};

// We call this recursively
void Linker::gather(std::unique_ptr<Node> pRoot,
                    const std::string& nameSpace) noexcept {
  // Add the node
  auto& root = *mLayout.emplace_back(std::move(pRoot), nameSpace).mNode.get();

  std::vector<std::unique_ptr<Node>> children;
  const auto result = root.getChildren(children);
  assert(result);

  for (auto& child : children)
    gather(std::move(child),
           (nameSpace.empty() ? "" : (nameSpace + "::")) + root.getId());

  if (!(root.getLinkingRestriction().Leaf)) {
    mLayout.emplace_back(std::make_unique<EndOfChildrenMarker>(root),
                         (nameSpace.empty() ? "" : (nameSpace + "::")) +
                             root.getId());
  }
}

void Linker::shuffle() {
  // TODO: Shuffle and fix
  // TODO: Namespace type + allow ID and name different lookup
}

void Linker::enforceRestrictions() {}

Result<void> Linker::write(Writer& writer, bool doShuffle, bool print_linkmap) {
  if (doShuffle) {
    shuffle();
    enforceRestrictions();
  }

  // Write data
  for (const auto& entry : mLayout) {
    // align
    u32 alignment = entry.mNode->getLinkingRestriction().alignment;
    if (alignment) {
      auto pad_begin = writer.tell();
      while (writer.tell() % alignment)
        writer.write('F', false);
      if (pad_begin != writer.tell() && mUserPad)
        mUserPad((char*)writer.getDataBlockStart() + pad_begin,
                 writer.tell() - pad_begin);
    }
    // Fill map: symbol and begin position
    mMap.push_back({(entry.mNamespace.empty() ? "" : entry.mNamespace + "::") +
                        entry.mNode->getId(),
                    writer.tell(), 0, entry.mNode->getLinkingRestriction()});
    // Write
    writer.mNameSpace = entry.mNamespace;
    writer.mBlockName = entry.mNode->getId();
    auto ok = entry.mNode->write(writer);
    if (!ok) {
      return std::unexpected(
          std::format("Linker failure: {} while writing node {}::{}",
                      ok.error(), entry.mNamespace, entry.mNode->getId()));
    }
    // Set ending position
    mMap[mMap.size() - 1].end = writer.tell();

    if (entry.mNode->getLinkingRestriction().PadEnd && alignment) {
      auto pad_begin = writer.tell();
      while (writer.tell() % alignment)
        writer.write('F', false);
      if (pad_begin != writer.tell() && mUserPad)
        mUserPad((char*)writer.getDataBlockStart() + pad_begin,
                 writer.tell() - pad_begin);
    }
  }

  if (print_linkmap) {
    printf("Begin    End      Size     Align    Static Leaf  Symbol\n");
    for (const auto& entry : mMap) {
      printf("0x%06x 0x%06x 0x%06x 0x%06x %s  %s %s\n", (u32)entry.begin,
             (u32)entry.end, (u32)(entry.end - entry.begin),
             (u32)entry.restrict.alignment,
             entry.restrict.Static ? "true " : "false",
             entry.restrict.Leaf ? "true " : "false", entry.symbol.c_str());
    }
  }

  // Resolve

  // TODO: map::ktpt::...::enpt is map::enpt
  for (const auto& reserve : writer.mLinkReservations) {
    // todo: make map
    const u32 addr = static_cast<u32>(reserve.addr);
    const Link& link = reserve.mLink;

    std::string fromBlockSymbol;
    std::string toBlockSymbol;

    // #ifdef BUILD_DEBUG
    const std::string& nameSpace =
        reserve.nameSpace.empty() ? "" : reserve.nameSpace + "::";

    // Order: local -> children -> global

    [[maybe_unused]] const Node& from =
        link.from.mBlock
            ? *link.from.mBlock
            : *LinkerHelper::findNamespacedID(*this, link.from.mId, nameSpace,
                                              reserve.blockName,
                                              fromBlockSymbol);
    [[maybe_unused]] const Node& to =
        link.to.mBlock
            ? *link.to.mBlock
            : *LinkerHelper::findNamespacedID(*this, link.to.mId, nameSpace,
                                              reserve.blockName, toBlockSymbol);
    // #endif
    //  TODO: Generalize all of these from/to methods
    if (link.from.mBlock) {
      bool bSuccess = false;
      for (const auto& entry : mLayout) {
        if (entry.mNode.get() == link.from.mBlock) {
          fromBlockSymbol =
              entry.mNamespace.empty()
                  ? entry.mNode->getId()
                  : entry.mNamespace + "::" + entry.mNode->getId();
          bSuccess = true;
          break;
        }
      }
      if (!bSuccess)
        printf("Linker Error: Block %s was never written to stream, so canot "
               "be resolved.\n",
               link.from.mBlock->getId().c_str());
    }
    if (link.to.mBlock) {
      bool bSuccess = false;
      for (const auto& entry : mLayout) {
        if (entry.mNode.get() == link.to.mBlock) {
          toBlockSymbol = entry.mNamespace.empty()
                              ? entry.mNode->getId()
                              : entry.mNamespace + "::" + entry.mNode->getId();
          bSuccess = true;
          break;
        }
      }
      if (!bSuccess)
        printf("Linker Error: Block %s was never written to stream, so canot "
               "be resolved.\n",
               link.to.mBlock->getId().c_str());
    }
    // TODO: Link: EndOfChildren + put that in map + if not all children static
    // and in shuffle, supply random number
    const u32 fromAddr = LinkerHelper::resolveHook(
        *this, fromBlockSymbol, link.from.mRelation, link.from.mOffset);
    const u32 toAddr = LinkerHelper::resolveHook(
        *this, toBlockSymbol, link.to.mRelation, link.to.mOffset);

    writer.seek<Whence::Set>(addr);

    u32 dif = (toAddr - fromAddr) / reserve.mLink.mStride;
    // writer.writeN(reserve.TSize, dif);
    switch (reserve.TSize) {
    case 1:
      EXPECT(dif < (u8)-1 && "Overflow error.");
      writer.write<u8>(dif);
      break;
    case 2:
      EXPECT(dif < (u16)-1 && "Overflow error.");
      writer.write<u16>(dif);
      break;
    case 4:
      EXPECT(dif < (u32)-1 && "Overflow error.");
      writer.write<u32>(dif);
      break;
    default:
      EXPECT(!"Invalid write size.");
      break;
    }
  }

  return {};
}

} // namespace oishii
