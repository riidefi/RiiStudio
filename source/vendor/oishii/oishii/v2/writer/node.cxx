/*!
 * @file
 * @brief Implementations for data blocks.
 */


#include "node.hxx"


namespace oishii::v2 {

Node::Result Node::gatherChildren(NodeDelegate& mOut) const
{
    return {};
}
Node::Result Node::getChildren(std::vector<std::unique_ptr<Node>>& mOut) const
{
    mOut.clear();
    NodeDelegate del{mOut};
    auto result = gatherChildren(del);

    if (!mOut.empty() && mLinkingRestriction.options & LinkingRestriction::Leaf)
    {
        mOut.clear();
        result = eResult::Warning;
        printf("Leaf node %s attempted to supply children.\n", getId().c_str());
    }

    return result;
}
} // namespace DataBlock
