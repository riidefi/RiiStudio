#pragma once

namespace kpi {

// Permute a persistent immutable document record, sharing memory where possible
static inline auto setNext(const auto& node, const auto* record) {
  return node.next(record);
}
// Restore a transient document node to a recorded state.
static inline void rollback(auto& node, const auto& record) {
  node.from(record);
}

} // namespace kpi
