#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

// Copied from cista.h: We don't want Windows.h imported when our custom macros are defined
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif


#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#endif

#include <cinttypes>
#include <memory>


#include <cstdlib>
#include <cstring>


#include <stdexcept>


#include <exception>

// End of copied region

#include <chrono>

#define throw
#define try if (1)
#define catch(...) else
#include <vendor/cista.h>
#undef throw
#undef try
#undef catch

#ifdef __clang__
#pragma clang diagnostic pop
#endif

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
