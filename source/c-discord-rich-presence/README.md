# c-discord-rich-presence
C/C++ bindings for the [discord-rich-presence crate](https://github.com/sardonicism-04/discord-rich-presence). Provides an alternative to the official Discord libraries for RPC for C++ applications.

The following structure will be filled out by the application:
```cpp
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
```

## Installation

### With CMake/[Corrosion](https://github.com/corrosion-rs/corrosion)
corrosion_import_crate(MANIFEST_PATH source/c-discord-rich-presence/Cargo.toml CRATE_TYPES staticlib)
### With other buildsystems
`cargo build` and copy-paste the compiled .lib (Windows) or .a (Linux/MacOS) library files into your project.

## Example: C++ application (using https://github.com/riidefi/RiiStudio/blob/master/source/rsl/Discord.hpp).
```c
#include `discord_rpc.h`

int main() {
	rsl::DiscordIpcClient client("YOUR APPLICATION ID");
	if (!client.connect()) {
		fprintf(stderr, "DiscordIpcClient failed to connect!\n");
		return -1;
	}
	
	discord_rpc::Activity my_activity;
	my_activity.state = "Hello README.md!";
	client.set_activity(my_activity);

	client.disconnect(); // Calling this is optional
}
```


## Example: Pure C application.
```cpp
#include `discord_rpc.h`

int main() {
	struct C_RPC* rpc = rsl_rpc_create("YOUR APPLICATION ID");

	int err = rsl_rpc_connect(rpc);
	if (err < 0) {
		fprintf(stderr, "rsl_rpc_connect failed!\n");
		return -1;
	}
	
	struct Activity_C my_activity;
	my_activity.state = "Hello README.md!";
	rsl_rpc_set_activity(rpc, &my_activity);

	rsl_rpc_disconnect(rpc);
	rsl_rpc_destroy(rpc);
}
```
