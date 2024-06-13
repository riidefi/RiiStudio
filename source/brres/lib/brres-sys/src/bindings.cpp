#include "bindings.h"

#include <string>
#include <vector>

#include "./librii/g3d/data/Archive.hpp"

#include <vendor/json_struct.h>

#include <LibBadUIFramework/Plugins.hpp>

bool gTestMode __attribute__((weak)) = false;

namespace librii::g3d {
struct DumpResult {
  std::string jsonData;
  std::vector<u8> collatedBuffer;
};

DumpResult DumpJson(const librii::g3d::Archive& archive);
Result<librii::g3d::Archive> ReadJsonArc(std::string_view json,
                                         std::span<const u8> buffers);
} // namespace librii::g3d

using DumpResult = librii::g3d::DumpResult;

extern "C" {

struct CResult {
  const char* json_metadata;
  u32 len_json_metadata;
  const void* buffer_data;
  u32 len_buffer_data;

  void (*freeResult)(CResult* self) = +[](CResult*) {};

  // Opaque to Rust
  DumpResult* opaque = nullptr;
};

} // extern "C"

static void SetCResult(CResult& result, DumpResult&& dumped) {
  delete result.opaque;
  DumpResult* impl = new DumpResult{std::move(dumped)};
  result.json_metadata = impl->jsonData.data();
  result.len_json_metadata = impl->jsonData.size();
  result.buffer_data = impl->collatedBuffer.data();
  result.len_buffer_data = impl->collatedBuffer.size();
  result.opaque = impl;
  result.freeResult = +[](CResult* self) {
    delete self->opaque;
    self->json_metadata = {};
    self->len_json_metadata = {};
    self->buffer_data = {};
    self->len_buffer_data = {};
    self->freeResult = +[](CResult*) {};
    self->opaque = {};
  };
}

struct Warning {
  JS_OBJ(mclass, domain, body);
  int mclass;
  std::string domain;
  std::string body;
};

struct ArchiveExtensions {
  JS_OBJ(warnings);
  std::vector<Warning> warnings;
};

extern "C" {

WASM_EXPORT u32 imp_brres_read_from_bytes(CResult* result, const void* buf,
                                          u32 len) {
  std::span<const u8> buf_span(reinterpret_cast<const u8*>(buf),
                               reinterpret_cast<const u8*>(buf) + len);

  ArchiveExtensions ext;

  kpi::LightIOTransaction trans;
  trans.callback = [&](kpi::IOMessageClass message_class,
                       const std::string_view domain,
                       const std::string_view message_body) {
    ext.warnings.push_back(Warning{
        static_cast<int>(message_class),
        std::string(domain),
        std::string(message_body),
    });
  };
  auto arc = librii::g3d::Archive::fromMemory(buf_span, "brres_read_from_bytes",
                                              trans);

  DumpResult dumped;
  bool ok = true;
  if (arc.has_value()) {
    dumped = librii::g3d::DumpJson(*arc);

    auto extJson = JS::serializeStruct(ext);

    // Assumes final character is a "}"
    assert(dumped.jsonData.size() >= 1);
    assert(dumped.jsonData[dumped.jsonData.size() - 1] == '}');
    dumped.jsonData.resize(dumped.jsonData.size() - 1);
    // Assumes inital character is a "{"
    assert(extJson.size() >= 1);
    assert(extJson[0] == '{');
    dumped.jsonData = dumped.jsonData + "," +  extJson.substr(1);
  } else {
    dumped.jsonData = arc.error();
    ok = false;
  }

  SetCResult(*result, std::move(dumped));

  return ok;
}

WASM_EXPORT void imp_brres_free(CResult* result) {
  if (result->freeResult)
    result->freeResult(result);
}

} // extern "C"

static Result<std::vector<u8>> WriteArchive(std::string_view json,
                                            std::span<const u8> blob) {
  auto arc = TRY(librii::g3d::ReadJsonArc(json, blob));
  auto bin = TRY(arc.write());

  return bin;
}

extern "C" {

WASM_EXPORT u32 imp_brres_write_bytes(CResult* result, const char* json,
                                      u32 json_len, const void* buffer,
                                      u32 buffer_len) {
  std::string_view json_view(json, json + json_len);
  std::span<const u8> buffer_span(reinterpret_cast<const u8*>(buffer),
                                  buffer_len);

  auto arc = WriteArchive(json_view, buffer_span);

  DumpResult dumped;
  bool ok = true;
  if (arc.has_value()) {
    dumped.jsonData = json;
    dumped.collatedBuffer = arc.value();
  } else {
    dumped.jsonData = arc.error();
    ok = false;
  }

  SetCResult(*result, std::move(dumped));

  return ok;
}

//
} // extern "C"
