#pragma once

#include <core/common.h>
#include <plugins/mk/BFG/FogEntry.hpp>

namespace riistudio::mk {

class BinaryFogData {
public:
  bool operator==(const BinaryFogData&) const = default;
};

struct NullClass {};

} // namespace riistudio::mk

#include <LibBadUIFramework/Node2.hpp>
#include <plugins/mk/BFG/Node.h>
