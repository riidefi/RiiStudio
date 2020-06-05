/*!
 * @file
 * @brief Data block nodes are the units for the new serialization model.
 */


#pragma once


#include "../types.hxx"
#include <vector>
#include <memory>
#include <string>

namespace oishii {

class Writer;

//! Restrictions for linkable blocks.
//!
struct LinkingRestriction
{
    enum Options
    {
        //! No bits are set.
        //!
        None = 0,

        //! A block must be linked behind the block in front of it.
        //! If this is not set, the linker is not required to maintain ordering.
        //!
        Static = (1 << 0),

        //! (TODO: Unimplemented) For unsigned offsets, datablocks cannot exist before their reference.
        //!
        Post = (1 << 1),

        //! Signifies that this datablock cannot have children.
        Leaf = (1 << 2)
    };

    Options options = None;

    //! Alignment of block. 0 to disable
    //!
    u32 alignment = 0;
};

//! @brief An abstract block (node) in the new serialization model.
//!
class Node
{
public:
    enum class eResult
    {
        Success, //!< Everything went okay.
        Warning, //!< Everything's not okay but we can still proceed.
        Fatal    //!< We failed. We might want to make this nuanced, denoting the effects.
    };

	//! @brief Default constructor.
	//!
	//! @param[in] id The ID of this node. Setting it blank signals a randomm unique ID to be generated.
	//!
	Node()
	{}

	//! @brief A constructor.
	//!
	//! @param[in] id The ID of this node. Setting it blank signals a randomm unique ID to be generated.
	//!
	explicit Node(const std::string& id)
		: mId(id)
	{}

	//! @brief A constructor.
	//!
	//! @param[in] linkRestrictions Restrictions for the linker.
	//!
	explicit Node(const LinkingRestriction& linkRestrictions)
		: mLinkingRestriction(linkRestrictions)
	{}

	//! @brief A constructor.
	//!
	//! @param[in] id The ID of this node. Setting it blank signals a randomm unique ID to be generated.
	//! @param[in] linkRestrictions Restrictions for the linker.
	//!
	Node(const std::string& id, const LinkingRestriction& linkRestrictions)
		: mId(id), mLinkingRestriction(linkRestrictions)
	{

	}
	//! @brief The destructor.
	//!
	~Node()
	{}


protected:
	std::string mId = ""; //!< String ID. If empty, linker will assign a unique ID.
	LinkingRestriction mLinkingRestriction;
	bool bLinkerOwned = false; //!< If this block should be freed upon linking completion.

public:
    //! @brief Write this block to a stream.
    //!
    //! @details The linker will call this for child blocks. Do not implement child block writing here.
    //!
    //! @param[in] writer Output stream.
    //!
    //! @return The result of the operation. Returning fatal will not stop other blocks from writing.
    //!
    virtual eResult write(Writer& writer) const noexcept = 0;
    
	// Deprecated
    //	//! @brief @return Return the formatted name of this resource.
    //	//!
    //	//! @details Default behavior will return "Untitled Datablock"
    //	//!
    //	virtual std::string getName() const noexcept;

protected:
    //! @brief Gathers children for the block. This will only be called through getChildren foreignly.
    //!
    //! @details Default behavior will do absolutely nothing.
    //!
    //! @param[out] mOut Vector of children to set. This will be empty when it is passed.
    //!
    //! @return The result of the operation. If this fails, the block will be treated as a leaf node.
    //!
    virtual eResult gatherChildren(std::vector<const Node*>& mOut) const;

public:
    //! @brief Get the children for this block.
    //!
    //! @param[out] mOut Vector of children to be set. This will be cleared.
    //!
    //! @return The result of the operation.
    //!     A leaf node returning children is considered a Warning and the children will be deleted.
    //!
    eResult getChildren(std::vector<const Node*>& mOut) const;

    inline const LinkingRestriction& getLinkingRestriction() const noexcept
    {
        return mLinkingRestriction;
    }

    inline LinkingRestriction& getLinkingRestriction() noexcept
    {
        return mLinkingRestriction;
    }

	inline bool isOwnedByLinker() const noexcept
	{
		return bLinkerOwned;
	}
	inline void transferOwnershipToLinker(bool state) noexcept
	{
		bLinkerOwned = state;
	}
	inline const std::string& getId() const noexcept
	{
		return mId;
	}
};

} // namespace DataBlock
