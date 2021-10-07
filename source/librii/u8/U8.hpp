#pragma once

#include <array>
#include <core/common.h>
#include <span>
#include <string>
#include <vector>

namespace librii::U8 {

struct U8Archive {
  struct Node {
    bool is_folder;
    std::string name;

    union {
      struct {
        u32 offset;
        u32 size;
      } file;
      struct {
        u32 parent;
        u32 sibling_next;
      } folder;
    };
  };
  std::array<u8, 16> watermark;
  std::vector<Node> nodes;
  std::vector<u8> file_data;
};

bool LoadU8Archive(U8Archive& result, std::span<const u8> data);
std::vector<u8> SaveU8Archive(const U8Archive& arc);

} // namespace librii::U8
