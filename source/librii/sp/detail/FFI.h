#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  char bdof[0x50];
} C_Settings;

enum {
  DEBUGCLIENT_RESULT_OK,
  DEBUGCLIENT_RESULT_TIMEOUT,
  DEBUGCLIENT_RESULT_OTHER,
};

// Rust: Spawn debug thread with tokio context, waiting for completion
void debugclient_init(void);
// Rust: Destroy debug thread and context, waiting on operations to close
void debugclient_deinit(void);
// Async
void debugclient_connect(const char* ip_port, int timeout,
                         void (*callback)(void* user, int result), void* user);
// Fire and forget
void debugclient_send_settings(const C_Settings* settings);
// Async
void debugclient_get_settings(int timeout,
                              void (*callback)(void* user, int result,
                                               const C_Settings* settings),
                              void* user);

#ifdef __cplusplus
}
#endif
