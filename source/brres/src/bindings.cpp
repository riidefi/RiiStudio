#include "bindings.h"

#include <string>
#include <vector>

extern "C" {

struct ResultImpl {
  std::string jsonData;
  std::vector<u8> collatedBuffer;
};

struct Result {
  const char* json_metadata;
  u32 len_json_metadata;
  const void* buffer_data;
  u32 len_buffer_data;

  void (*freeResult)(Result* self);

  // Opaque to Rust
  ResultImpl* opaque;
};

WASM_EXPORT void brres_read_from_bytes(Result* result, const void* buf,
                                       u32 len) {
  std::string jsonData = "{\"models\": [{ \"name\": \"tmp\" }]}";
  std::vector<u8> collatedBuffer;

  ResultImpl* impl =
      new ResultImpl{std::move(jsonData), std::move(collatedBuffer)};
  result->json_metadata = impl->jsonData.data();
  result->len_json_metadata = impl->jsonData.size();
  result->buffer_data = impl->collatedBuffer.data();
  result->len_buffer_data = impl->collatedBuffer.size();
  result->opaque = impl;
  result->freeResult = +[](Result* self) {
    delete self->opaque;
    self->json_metadata = {};
    self->len_json_metadata = {};
    self->buffer_data = {};
    self->len_buffer_data = {};
    self->freeResult = +[](Result*) {};
    self->opaque = {};
  };
}

WASM_EXPORT void brres_free(Result* result) { result->freeResult(result); }

//
}
