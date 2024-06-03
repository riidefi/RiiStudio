#include "bindings.h"

#include <string>
#include <vector>

#include "../../librii/g3d/data/Archive.hpp"

bool gTestMode = false;

struct DumpResult {
  std::string jsonData;
  std::vector<u8> collatedBuffer;
};

DumpResult DumpJson(const librii::g3d::Archive& archive);

extern "C" {

struct CResult {
  const char* json_metadata;
  u32 len_json_metadata;
  const void* buffer_data;
  u32 len_buffer_data;

  void (*freeResult)(CResult* self);

  // Opaque to Rust
  DumpResult* opaque;
};

WASM_EXPORT u32 brres_read_from_bytes(CResult* result, const void* buf,
                                       u32 len) {
  std::span<const u8> buf_span(reinterpret_cast<const u8*>(buf),
                               reinterpret_cast<const u8*>(buf) + len);
  auto arc =
      librii::g3d::Archive::fromMemory(buf_span, "brres_read_from_bytes");

  DumpResult dumped;
  bool ok = true;
  if (arc.has_value()) {
    dumped = DumpJson(*arc);
  } else {
    dumped.jsonData = arc.error();
    ok = false;
  }

  DumpResult* impl = new DumpResult{std::move(dumped.jsonData),
                                    std::move(dumped.collatedBuffer)};
  result->json_metadata = impl->jsonData.data();
  result->len_json_metadata = impl->jsonData.size();
  result->buffer_data = impl->collatedBuffer.data();
  result->len_buffer_data = impl->collatedBuffer.size();
  result->opaque = impl;
  result->freeResult = +[](CResult* self) {
    delete self->opaque;
    self->json_metadata = {};
    self->len_json_metadata = {};
    self->buffer_data = {};
    self->len_buffer_data = {};
    self->freeResult = +[](CResult*) {};
    self->opaque = {};
  };

  return ok;
}

WASM_EXPORT void brres_free(CResult* result) { result->freeResult(result); }

//
}
