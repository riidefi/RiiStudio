#pragma once

#include <array>
#include <core/common.h>
#include <rsl/SimpleReader.hpp>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace librii::U8 {

struct U8Archive {
  struct Node {
    bool is_folder = false;
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
  std::array<u8, 16> watermark{};
  std::vector<Node> nodes;
  std::vector<u8> file_data;
};

bool IsDataU8Archive(rsl::byte_view data);

Result<U8Archive> LoadU8Archive(rsl::byte_view data);
std::vector<u8> SaveU8Archive(const U8Archive& arc);

//! Get the Node associated with a certain path, or -1.
//!
//! Highly accurate function to game behavior.
s32 PathToEntrynum(const U8Archive& arc, const char* path, u32 currentPath = 0);

Result<void> Extract(const U8Archive& arc, std::filesystem::path out);
Result<U8Archive> Create(std::filesystem::path root);

} // namespace librii::U8
