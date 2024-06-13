#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <rsl/SafeReader.hpp>

namespace librii::g3d {

// Only exposes what you need. Use this.
struct BetterNode {
  std::string name{};
  unsigned int stream_pos{};

  bool operator==(const BetterNode&) const = default;
};

struct BetterDictionary {
  std::vector<BetterNode> nodes;

  BetterDictionary() = default;
  BetterDictionary(const BetterDictionary&) = default;
  BetterDictionary(BetterDictionary&&) = default;
  BetterDictionary(size_t n) : nodes(n) {}

  bool operator==(const BetterDictionary&) const = default;
};

int CalcDictionarySize(const BetterDictionary& dict);
int CalcDictionarySize(int num_nodes);
Result<BetterDictionary> ReadDictionary(rsl::SafeReader& reader);
void WriteDictionary(const BetterDictionary& dict, oishii::Writer& writer,
                     NameTable& names);

} // namespace librii::g3d
