#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace kpi {

struct IMemento;
struct INode;

// Permute a persistent immutable document record, sharing memory where possible
std::shared_ptr<const IMemento> setNext(const INode& node,
                                        const IMemento* record);
// Restore a transient document node to a recorded state.
void rollback(INode& node, const IMemento& record);

} // namespace kpi
