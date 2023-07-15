#pragma once

#include <stdint.h>
#ifdef __cplusplus
#include <string.h>
#include <string>
#include <vector>
#endif // __cplusplus

#ifdef __cplusplus
namespace discord_rpc {
struct Activity {
  std::string state = "A test";
  std::string details = "A placeholder";
  struct Timestamps {
    int64_t start = 0;
    int64_t end = 0;
  };
  Timestamps timestamps;
  struct Assets {
    std::string large_image = "large-image";
    std::string large_text = "Large text";
  };
  Assets assets;
  struct Button {
    std::string text = "A button";
    std::string link = "https://github.com";
  };
  std::vector<Button> buttons{Button{}};
};
} // namespace discord_rpc
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

struct Timestamps_C {
  int64_t start;
  int64_t end;
};

struct Assets_C {
  const char* large_image;
  const char* large_text;
};

struct Button_C {
  const char* text;
  const char* link;
};

struct ButtonVector_C {
  struct Button_C* buttons;
  size_t length;
};

struct Activity_C {
  const char* state;
  const char* details;
  struct Timestamps_C timestamps;
  struct Assets_C assets;
  struct ButtonVector_C buttons;
};

//! Opaque class representation a RPC socket.
struct C_RPC;

struct C_RPC* rsl_rpc_create(const char* application_id);
void rsl_rpc_destroy(struct C_RPC*);

// 0 -> OK, negative -> error
int rsl_rpc_connect(struct C_RPC*);
// Only valid if prior connect call succeeded
void rsl_rpc_disconnect(struct C_RPC*);
// Sets a status.
void rsl_rpc_set_activity(struct C_RPC*, struct Activity_C* activity);
// Sets dummy status
void rsl_rpc_test(struct C_RPC*);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace discord_rpc {
// Functions to create and destroy the Activity_C struct and to access the
// fields of the Activity struct
inline Activity_C* create_activity(const Activity& activity) {
  Activity_C* activity_c = new Activity_C;

  // Allocate and copy strings for state and details
  activity_c->state = strdup(activity.state.c_str());
  activity_c->details = strdup(activity.details.c_str());

  // Copy timestamps and assets
  activity_c->timestamps.start = activity.timestamps.start;
  activity_c->timestamps.end = activity.timestamps.end;
  activity_c->assets.large_image = strdup(activity.assets.large_image.c_str());
  activity_c->assets.large_text = strdup(activity.assets.large_text.c_str());

  // Allocate and copy button vector
  activity_c->buttons.length = activity.buttons.size();
  activity_c->buttons.buttons = new Button_C[activity_c->buttons.length];
  for (size_t i = 0; i < activity_c->buttons.length; i++) {
    activity_c->buttons.buttons[i].text =
        strdup(activity.buttons[i].text.c_str());
    activity_c->buttons.buttons[i].link =
        strdup(activity.buttons[i].link.c_str());
  }

  return activity_c;
}

inline void destroy_activity(Activity_C* activity_c) {
  // Free strings
  free((void*)activity_c->state);
  free((void*)activity_c->details);
  free((void*)activity_c->assets.large_image);
  free((void*)activity_c->assets.large_text);

  // Free button vector
  for (size_t i = 0; i < activity_c->buttons.length; i++) {
    free((void*)activity_c->buttons.buttons[i].text);
    free((void*)activity_c->buttons.buttons[i].link);
  }
  delete[] activity_c->buttons.buttons;

  delete activity_c;
}
} // namespace discord_rpc
#endif // __cplusplus
