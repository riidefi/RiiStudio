/*!
 * @file
 * @brief Data block nodes are the units for the new serialization model.
 */

#pragma once

#include "../types.hxx"
#include <core/common.h>

namespace oishii {

class Writer;

//! Restrictions for linkable blocks.
//!
struct LinkingRestriction {
  //! A block must be linked behind the block in front of it.
  //! If this is not set, the linker is not required to maintain ordering.
  //!
  bool Static : 1 = false;

  //! (TODO: Unimplemented) For unsigned offsets, datablocks cannot exist
  //! before their reference.
  //!
  bool Post : 1 = false;

  //! Signifies that this datablock cannot have children.
  //!
  bool Leaf : 1 = false;

  //! Pad the end of the block as well as the start.
  //!
  bool PadEnd : 1 = false;

  //! Alignment of block. 0 to disable
  //!
  u32 alignment = 0;

  void setLeaf() { Leaf = true; }
};

//! @brief An abstract block (node) in the new serialization model.
//!
class Node {
public:
  enum class eResult {
    Success, //!< Everything went okay.
    Warning, //!< Everything's not okay but we can still proceed.
    Fatal    //!< We failed. We might want to make this nuanced, denoting the
             //!< effects.
  };

  struct Result {
    Result() : _e(eResult::Success) {}
    Result(eResult e) : _e(e) {}

    operator eResult() const { return _e; }

    operator std::expected<void, std::string>() const {
      if (_e != eResult::Success) {
        return std::unexpected("Node error");
      }
      return {};
    }

    eResult _e;
  };

  //! @brief The destructor.
  //!
  virtual ~Node() = default;

  //! @brief Default constructor.
  //!
  Node() = default;

  //! @brief A constructor.
  //!
  //! @param[in] id The ID of this node. Setting it blank signals a randomm
  //! unique ID to be generated.
  //!
  explicit Node(const std::string& id) : mId(id) {}

  //! @brief A constructor.
  //!
  //! @param[in] linkRestrictions Restrictions for the linker.
  //!
  explicit Node(const LinkingRestriction& linkRestrictions)
      : mLinkingRestriction(linkRestrictions) {}

  //! @brief A constructor.
  //!
  //! @param[in] id The ID of this node. Setting it blank signals a randomm
  //! unique ID to be generated.
  //! @param[in] linkRestrictions Restrictions for the linker.
  //!
  Node(const std::string& id, const LinkingRestriction& linkRestrictions)
      : mId(id), mLinkingRestriction(linkRestrictions) {}

protected:
  std::string mId =
      ""; //!< String ID. If empty, linker will assign a unique ID.
  LinkingRestriction mLinkingRestriction;

public:
  //! @brief Write this block to a stream.
  //!
  //! @details The linker will call this for child blocks. Do not implement
  //! child block writing here.
  //!
  //! @param[in] writer Output stream.
  //!
  //! @return The result of the operation. Returning fatal will not stop other
  //! blocks from writing.
  //!
  virtual Result write([[maybe_unused]] Writer& writer) const noexcept {
    return {};
  }
  virtual std::expected<void, std::string>
  write2(Writer& writer) const noexcept {
    return write(writer);
  }

public:
  struct NodeDelegate {
    void addNode(std::unique_ptr<Node> node) {
      mVec.push_back(std::move(node));
    }

    NodeDelegate(std::vector<std::unique_ptr<Node>>& vec) : mVec(vec) {}

  private:
    std::vector<std::unique_ptr<Node>>& mVec;
  };

protected:
  //! @brief Gathers children for the block. This will only be called through
  //! getChildren foreignly.
  //!
  //! @details Default behavior will do absolutely nothing.
  //!
  //! @param[out] mOut Delegate for adding nodes.
  //!
  //! @return The result of the operation. If this fails, the block will be
  //! treated as a leaf node.
  //!
  virtual Result gatherChildren(NodeDelegate& mOut) const;

public:
  //! @brief Get the children for this block.
  //!
  //! @param[out] mOut Vector of children to be set. This will be cleared.
  //!
  //! @return The result of the operation.
  //!     A leaf node returning children is considered a Warning and the
  //!     children will be deleted.
  //!
  Result getChildren(std::vector<std::unique_ptr<Node>>& mOut) const;

  inline const LinkingRestriction& getLinkingRestriction() const noexcept {
    return mLinkingRestriction;
  }

  inline LinkingRestriction& getLinkingRestriction() noexcept {
    return mLinkingRestriction;
  }

  inline const std::string& getId() const noexcept { return mId; }

  Node* toLeaf() {
    mLinkingRestriction.Leaf = true;
    return this;
  }
  Node* toStatic() {
    mLinkingRestriction.Static = true;
    return this;
  }
};

struct LeafNode : public Node {
  LeafNode() : Node(LinkingRestriction{.Leaf = true}) {}
};

} // namespace oishii
