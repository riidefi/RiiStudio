#pragma once

#include <format>
#include <oishii/reader/binary_reader.hxx>
#include <rsl/Ranges.hpp>
#include <sstream>
#include <vendor/magic_enum/magic_enum.hpp>

namespace rsl {

inline std::string EnumError(u32 bad, auto&& good) {
  auto values_printed =
      rsl::join(good | std::views::transform([](auto x) {
                  auto [id, name] = x;
                  auto id_raw = static_cast<u32>(id);
                  return std::format("{}={}=0x{:x}", name, id_raw, id_raw);
                }),
                ", ");
  return std::format(
      "Invalid enum value. Expected one of ({}). Instead saw {} (0x{:x}).",
      values_printed, bad, bad);
}
class SafeReader {
public:
  SafeReader(oishii::BinaryReader& reader) : mReader(reader) {}

  auto U32() { return mReader.tryRead<u32>(); }
  auto U16() { return mReader.tryRead<u16>(); }
  template <typename E> rsl::expected<E, std::string> Enum32() {
    auto u = TRY(mReader.tryRead<u32>());
    auto as_enum = magic_enum::enum_cast<E>(u);
    if (!as_enum.has_value()) {
      // TODO: Allow warn-at to return a string
      auto values = magic_enum::enum_entries<E>();
      auto msg = EnumError(u, values);
      mReader.warnAt(msg.c_str(), mReader.tell() - 4, mReader.tell());
      return msg;
    }
    return static_cast<E>(u);
  }

  std::string StringOfs32(u32 relative) {
    auto ofs = TRY(mReader.tryRead<s32>());

    if (ofs == 0) {
      return "";
    }

    if (ofs + relative >= mReader.endpos() && ofs + relative < 0) {
      return std::format("Invalid string offset {}. Out of file bounds [0, {})",
                         ofs + relative, mReader.endpos());
    }

    auto span = mReader.slice();
    for (size_t i = 0; i < span.size(); ++i) {
      if (span[i] == 0) {
        span = span.subspan(0, i);
        break;
      }
    }

    return std::string{
        reinterpret_cast<const char*>(span.data()),
        reinterpret_cast<const char*>(span.data() + span.size())};
  }

  auto scoped(std::string&& name) {
    return mReader.createScoped(std::move(name));
  }

private:
  oishii::BinaryReader& mReader;
};

} // namespace rsl
