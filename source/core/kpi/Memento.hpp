#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace kpi {

struct INode;
struct IDocData;

struct DocumentMemento {
  std::shared_ptr<const DocumentMemento> parent = nullptr;
  std::map<std::string, std::vector<std::shared_ptr<const DocumentMemento>>>
      children;
  std::set<std::string> lut;
  std::shared_ptr<const IDocData> JustData = nullptr;

  bool operator==(const DocumentMemento& rhs) const {
    return parent == rhs.parent && children == rhs.children && lut == rhs.lut &&
           JustData == rhs.JustData;
  }
};

// Permute a persistent immutable document record, sharing memory where possible
std::shared_ptr<const DocumentMemento>
setNext(const INode& node, std::shared_ptr<const DocumentMemento> record);
// Restore a transient document node to a recorded state.
void rollback(INode& node, std::shared_ptr<const DocumentMemento> record);

} // namespace kpi
