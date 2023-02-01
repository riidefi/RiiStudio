#pragma once

#include <core/common.h> // assert

namespace imcxx {

// Over some ranges [0, n)
//
// Node0     [indent=0
//     Node1 [indent=1]
//         Node2 [indent=2]
//     Node3 [indent=3]
//
// From a tree structure, nodes must be supplied as a DFS.
// The advantages of this approach:
// - Selection math is simple as the nodes are already in DFS order
// - No recursion needed; data can be stored in a linear array.
//
class IndentedTreeWidget {
public:
  virtual int Indent(size_t i) const = 0;
  virtual bool Enabled(size_t i) const = 0;
  virtual bool DrawNode(size_t i, size_t filteredIndex, bool hasChild) = 0;
  virtual void CloseNode() = 0;
  virtual int NumNodes() const = 0;
  void DrawIndentedTree() {
    // A filter tree for multi selection. Prevents inclusion of unfiltered data
    // with SHIFT clicks.
    const int count = NumNodes();
    int depth = 0;
    size_t numFiltered = 0;
    for (int i = 0; i < count; ++i) {
      if (!Enabled(i)) {
        continue;
      }
      bool has_child = i + 1 < count && Indent(i + 1) > Indent(i);
      bool treenode = DrawNode(i, numFiltered++, has_child);
      if (treenode)
        depth++;
      int next;
      if (i + 1 >= count) {
        next = 0;
      } else {
        next = Indent(i + 1);
      }
      if (!treenode) {
        int indent = Indent(i);
        if (next > indent) {
          ++i;
          while (i < count && Indent(i) > indent) {
            ++i;
          }
          if (i + 1 >= count) {
            next = 0;
          } else {
            next = Indent(i);
          }
          --i;
        }
      }
      for (int j = depth; j > next; --j) {
        CloseNode();
        --depth;
      }
    }
    assert(depth == 0);
  }
};

} // namespace imcxx
