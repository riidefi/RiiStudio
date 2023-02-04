#pragma once

#include <vendor/cista.h>

#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>

namespace rsl {

template <typename T> Result<void> ReadFields(T& out, rsl::SafeReader& reader) {
  std::optional<std::string> err;
  cista::for_each_field(out, [&](auto&& x) {
    if (err.has_value()) {
      return;
    }
    auto ok = reader.getUnsafe().tryRead<std::remove_cvref_t<decltype(x)>>();
    if (!ok) {
      err = ok.error();
      return;
    }
    x = *ok;
  });
  if (err.has_value()) {
    return std::unexpected(*err);
  }
  return {};
}
template <typename T> void WriteFields(oishii::Writer& writer, const T& obj) {
  cista::for_each_field(obj, [&](auto&& x) { writer.write(x); });
}

} // namespace rsl
