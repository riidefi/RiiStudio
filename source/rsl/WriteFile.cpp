#include "WriteFile.hpp"
#include <fstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "Log.hpp"

namespace rsl {

// In EMSCRIPTEN, we cannot directly write to the user's filesystem easily
// (although there is a web fs API we don't use). Instead, we put up the file
// for download. This depends on a "downloadBuffer" function, implemented in
// index.html as such:
//
//    downloadBlob: function (filename, blob) {
//        const url = window.URL.createObjectURL(blob);
//        window.Module.downloadHref(filename, url);
//        window.URL.revokeObjectURL(url);
//    },
//    downloadBuffer : function(ptr, size, string_ptr, string_size) {
//        let string_buf = new Uint8Array(window.Module.HEAPU8.buffer,
//									      string_ptr,
//                                        string_size);
//        let filename = String.fromCharCode.apply(String, string_buf);
//        let buf = new Uint8Array(window.Module.HEAPU8.buffer, ptr, size);
//        window.Module.downloadBlob(filename, new Blob([buf]));
//    },
//
// It may be better to implement this directly in the file instead...
//
#if !defined(__EMSCRIPTEN__)
Result<void> WriteFile(const std::span<const uint8_t> data,
                       const std::string_view path) {
  std::ofstream stream(std::string(path), std::ios::binary | std::ios::out);
  stream.write(reinterpret_cast<const char*>(data.data()), data.size());
  return {};
}
#else
Result<void> WriteFile(const std::span<const uint8_t> data,
                       const std::string_view path) {
  rsl::info("WriteFile...");
  static_assert(sizeof(void*) == sizeof(uint32_t), "emscripten pointer size");

  EM_ASM({ window.Module.downloadBuffer($0, $1, $2, $3); },
         reinterpret_cast<uint32_t>(data.data()), data.size(),
         reinterpret_cast<uint32_t>(path.data()), path.size());
  return {};
}
#endif

} // namespace rsl
