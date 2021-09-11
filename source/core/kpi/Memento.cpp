#include "Node2.hpp"
#include "Reflection.hpp"

namespace kpi {

std::shared_ptr<const IMemento> setNext(const IMementoOriginator& node,
                                        const IMemento* record) {
  return node.next(record);
}

void rollback(IMementoOriginator& node, const IMemento& record) {
  node.from(record);
}

} // namespace kpi
