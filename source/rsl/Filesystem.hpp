// C++23 filesystem wrappers with std::expected

#pragma once

#include <filesystem>
#include <rsl/Expected.hpp>

namespace rsl::filesystem {

std::expected<std::filesystem::path, std::error_code>
absolute(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::absolute(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}
std::expected<std::filesystem::path, std::error_code>
canonical(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::canonical(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::path, std::error_code>
weakly_canonical(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::weakly_canonical(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::path, std::error_code>
relative(const std::filesystem::path& p,
         const std::filesystem::path& base = std::filesystem::current_path()) {
  std::error_code ec;
  auto result = std::filesystem::relative(p, base, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::path, std::error_code>
proximate(const std::filesystem::path& p,
          const std::filesystem::path& base = std::filesystem::current_path()) {
  std::error_code ec;
  auto result = std::filesystem::proximate(p, base, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<void, std::error_code>
copy(const std::filesystem::path& from, const std::filesystem::path& to,
     std::filesystem::copy_options options =
         std::filesystem::copy_options::none) {
  std::error_code ec;
  std::filesystem::copy(from, to, options, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<bool, std::error_code>
copy_file(const std::filesystem::path& from, const std::filesystem::path& to,
          std::filesystem::copy_options options =
              std::filesystem::copy_options::none) {
  std::error_code ec;
  auto result = std::filesystem::copy_file(from, to, options, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<void, std::error_code>
copy_symlink(const std::filesystem::path& existing_symlink,
             const std::filesystem::path& new_symlink) {
  std::error_code ec;
  std::filesystem::copy_symlink(existing_symlink, new_symlink, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<bool, std::error_code>
create_directory(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::create_directory(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
create_directories(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::create_directories(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<void, std::error_code>
create_hard_link(const std::filesystem::path& to,
                 const std::filesystem::path& new_hard_link) {
  std::error_code ec;
  std::filesystem::create_hard_link(to, new_hard_link, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<void, std::error_code>
create_symlink(const std::filesystem::path& to,
               const std::filesystem::path& new_symlink) {
  std::error_code ec;
  std::filesystem::create_symlink(to, new_symlink, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<void, std::error_code>
create_directory_symlink(const std::filesystem::path& to,
                         const std::filesystem::path& new_symlink) {
  std::error_code ec;
  std::filesystem::create_directory_symlink(to, new_symlink, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<std::filesystem::path, std::error_code> current_path() {
  std::error_code ec;
  auto result = std::filesystem::current_path(ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code> exists(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::exists(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
equivalent(const std::filesystem::path& p1, const std::filesystem::path& p2) {
  std::error_code ec;
  auto result = std::filesystem::equivalent(p1, p2, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<uintmax_t, std::error_code>
file_size(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::file_size(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<uintmax_t, std::error_code>
hard_link_count(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::hard_link_count(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::file_time_type, std::error_code>
last_write_time(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::last_write_time(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<void, std::error_code>
permissions(const std::filesystem::path& p, std::filesystem::perms prms,
            std::filesystem::perm_options opts =
                std::filesystem::perm_options::replace) {
  std::error_code ec;
  std::filesystem::permissions(p, prms, opts, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}
std::expected<std::filesystem::path, std::error_code>
read_symlink(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::read_symlink(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code> remove(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::remove(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<uintmax_t, std::error_code>
remove_all(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::remove_all(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<void, std::error_code> rename(const std::filesystem::path& from,
                                            const std::filesystem::path& to) {
  std::error_code ec;
  std::filesystem::rename(from, to, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<void, std::error_code> resize_file(const std::filesystem::path& p,
                                                 uintmax_t size) {
  std::error_code ec;
  std::filesystem::resize_file(p, size, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return {};
}

std::expected<std::filesystem::space_info, std::error_code>
space(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::space(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::file_status, std::error_code>
status(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::status(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::file_status, std::error_code>
symlink_status(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::symlink_status(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<std::filesystem::path, std::error_code> temp_directory_path() {
  std::error_code ec;
  auto result = std::filesystem::temp_directory_path(ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}
std::expected<bool, std::error_code>
is_block_file(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_block_file(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
is_character_file(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_character_file(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
is_directory(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_directory(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code> is_empty(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_empty(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code> is_fifo(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_fifo(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code> is_other(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_other(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
is_regular_file(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_regular_file(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code> is_socket(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_socket(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
is_symlink(const std::filesystem::path& p) {
  std::error_code ec;
  auto result = std::filesystem::is_symlink(p, ec);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}

std::expected<bool, std::error_code>
status_known(const std::filesystem::file_status& s) {
  std::error_code ec;
  auto result = std::filesystem::status_known(s);
  if (ec) {
    return std::unexpected(ec);
  }
  return result;
}
} // namespace rsl::filesystem
