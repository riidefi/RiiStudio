/*!
 * @file
 * @brief Implementations for data blocks.
 */

#include "node.hxx"
#include "oishii/interfaces.hxx"

#include <fstream>

namespace oishii {

Node::Result Node::gatherChildren([[maybe_unused]] NodeDelegate& mOut) const {
  return {};
}
Node::Result Node::getChildren(std::vector<std::unique_ptr<Node>>& mOut) const {
  mOut.clear();
  NodeDelegate del{mOut};
  auto result = gatherChildren(del);

  if (!mOut.empty() && mLinkingRestriction.Leaf) {
    mOut.clear();
    result = eResult::Warning;
    printf("Leaf node %s attempted to supply children.\n", getId().c_str());
  }

  return result;
}

void OishiiDefaultFlushFile(std::span<const u8> buf, std::string_view path) {
  std::ofstream stream(std::string(path), std::ios::binary | std::ios::out);
  stream.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}
FlushFileHandler s_flushFileHandler = OishiiDefaultFlushFile;

void SetGlobalFileWriteFunction(FlushFileHandler handler) {
  s_flushFileHandler = handler;
}

void FlushFile(std::span<const u8> buf, std::string_view path) {
  assert(s_flushFileHandler != nullptr);
  s_flushFileHandler(buf, path);
}

} // namespace oishii
