#include "Node.hpp"
#include "Reflection.hpp"

namespace kpi {

std::shared_ptr<const IMemento> setNext(const INode& node,
                                        const IMemento* record) {
  return node.next(record);
}

void rollback(INode& node, const IMemento& record) { node.from(record); }

} // namespace kpi
