/*!
 * @file
 * @brief Data block nodes are the units for the new serialization model.
 */

#pragma once

#include <memory>
#include <rsl/Expected.hpp>
#include <rsl/Types.hpp>
#include <string>
#include <vector>

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
  virtual std::expected<void, std::string>
  write(Writer& writer) const noexcept {
    return {};
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
  virtual std::expected<void, std::string>
  gatherChildren(NodeDelegate& mOut) const;

public:
  //! @brief Get the children for this block.
  //!
  //! @param[out] mOut Vector of children to be set. This will be cleared.
  //!
  //! @return The result of the operation.
  //!     A leaf node returning children is considered a Warning and the
  //!     children will be deleted.
  //!
  std::expected<void, std::string>
  getChildren(std::vector<std::unique_ptr<Node>>& mOut) const;

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
