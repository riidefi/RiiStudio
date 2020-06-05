/*!
 * @file
 * @brief Implementations for data blocks.
 */


#include "node.hxx"


namespace oishii {

//std::string Node::getName() const noexcept
//{
//    return "Untitled Datablock";
//}
Node::eResult Node::gatherChildren(std::vector<const Node*>& mOut) const
{
    return eResult::Success;
}
Node::eResult Node::getChildren(std::vector<const Node*>& mOut) const
{
    mOut.clear();
    auto result = gatherChildren(mOut);

    if (!mOut.empty() && mLinkingRestriction.options & LinkingRestriction::Leaf)
    {
        mOut.clear();
        result = eResult::Warning;
        printf("Leaf node %s attempted to supply children.\n", getId().c_str());
    }

    return result;
}
} // namespace DataBlock