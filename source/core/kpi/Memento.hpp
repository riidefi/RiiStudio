#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace kpi {

struct IMemento;
struct INode;
struct IMementoOriginator;

// Permute a persistent immutable document record, sharing memory where possible
std::shared_ptr<const IMemento> setNext(const IMementoOriginator& node,
                                        const IMemento* record);
// Restore a transient document node to a recorded state.
void rollback(IMementoOriginator& node, const IMemento& record);

} // namespace kpi
