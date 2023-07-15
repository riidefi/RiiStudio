use discord_rich_presence::{activity, DiscordIpc, DiscordIpcClient};
use log::*;
use std::ffi::c_char;
use std::ffi::c_longlong;
use std::ffi::CStr;
use std::slice;

fn rpc_create(app_id: &str) -> Result<DiscordIpcClient, Box<dyn std::error::Error>> {
    warn!("[DiscordIpcClient] Creating client");
    DiscordIpcClient::new(app_id)
}
fn rpc_connect(client: &mut DiscordIpcClient) -> i32 {
    warn!("[DiscordIpcClient] Connecting...");
    match client.connect() {
        Ok(_) => 0,
        Err(err) => {
            let msg = err.to_string();
            error!("[DiscordIpcClient] {msg}");
            -1
        }
    }
}

#[no_mangle]
pub extern "C" fn rsl_rpc_disconnect(client: &mut DiscordIpcClient) {
    warn!("[DiscordIpcClient] Closing connection...");
    match client.close() {
        Ok(_) => (),
        Err(err) => {
            let msg = err.to_string();
            error!("[DiscordIpcClient] {msg}");
        }
    }
}

fn rpc_test(client: &mut DiscordIpcClient) -> Result<(), Box<dyn std::error::Error>> {
    warn!("[DiscordIpcClient] Setting activity...");
    let activity = activity::Activity::new()
        .state("A test")
        .details("A placeholder")
        .assets(
            activity::Assets::new()
                .large_image("large-image")
                .large_text("Large text"),
        )
        .buttons(vec![activity::Button::new(
            "A button",
            "https://github.com",
        )]);
    client.set_activity(activity)?;
    Ok(())
}

#[no_mangle]
pub extern "C" fn rsl_rpc_test(client: &mut DiscordIpcClient) {
    warn!("[DiscordIpcClient] rsl_rpc_test()");
    let _ = rpc_test(client);
}

#[no_mangle]
pub extern "C" fn rsl_rpc_connect(client: &mut DiscordIpcClient) -> i32 {
    warn!("[DiscordIpcClient] rsl_rpc_connect()");
    rpc_connect(client)
}

#[no_mangle]
pub extern "C" fn rsl_rpc_create(s: *const c_char) -> *mut DiscordIpcClient {
    let app_id = unsafe { String::from_utf8_lossy(CStr::from_ptr(s).to_bytes()) };
    match rpc_create(&app_id) {
        Ok(client) => {
            // Return an owning pointer to C
            let boxed = Box::new(client);
            Box::into_raw(boxed) as *mut _
        }
        Err(err) => {
            let msg = err.to_string();
            error!("[DiscordIpcClient] {msg}");
            std::ptr::null::<DiscordIpcClient>() as *mut _
        }
    }
}
#[no_mangle]
pub extern "C" fn rsl_rpc_destroy(client: *mut DiscordIpcClient) {
    warn!("[DiscordIpcClient] Destroying client");
    unsafe {
        let mut _owning_boxed = Box::from_raw(client);
    }
}
#[repr(C)]
pub struct Timestamps {
    pub start: c_longlong,
    pub end: c_longlong,
}

#[repr(C)]
pub struct Assets {
    pub large_image: *const c_char,
    pub large_text: *const c_char,
}

#[repr(C)]
pub struct Button {
    pub text: *const c_char,
    pub link: *const c_char,
}

#[repr(C)]
pub struct ButtonVector {
    pub buttons: *mut Button,
    pub length: usize,
}

#[repr(C)]
pub struct ActivityC {
    pub state: *const c_char,
    pub details: *const c_char,
    pub timestamps: Timestamps,
    pub assets: Assets,
    pub buttons: ButtonVector,
}
pub mod ffi {
    pub struct Asset {
        pub large_image: String,
        pub large_text: String,
    }
    pub struct Button {
        pub text: String,
        pub link: String,
    }
    pub struct Timestamps {
        pub start: i64,
        pub end: i64,
    }
    pub struct Activity {
        pub state: String,
        pub details: String,
        pub timestamps: Timestamps,

        pub assets: Asset,
        pub buttons: Vec<Button>,
    }
}
impl From<&ActivityC> for ffi::Activity {
    fn from(activity_c: &ActivityC) -> Self {
        // Helper function to convert C string into Rust String
        fn c_str_to_string(c_str: *const c_char) -> String {
            unsafe { CStr::from_ptr(c_str) }
                .to_string_lossy()
                .into_owned()
        }

        // Convert button vector to Rust Vec<Button>
        let button_slice =
            unsafe { slice::from_raw_parts(activity_c.buttons.buttons, activity_c.buttons.length) };
        let buttons = button_slice
            .iter()
            .map(|button_c| ffi::Button {
                text: c_str_to_string(button_c.text),
                link: c_str_to_string(button_c.link),
            })
            .collect();

        // Construct the Rust Activity struct
        ffi::Activity {
            state: c_str_to_string(activity_c.state),
            details: c_str_to_string(activity_c.details),
            timestamps: ffi::Timestamps {
                start: activity_c.timestamps.start,
                end: activity_c.timestamps.end,
            },
            assets: ffi::Asset {
                large_image: c_str_to_string(activity_c.assets.large_image),
                large_text: c_str_to_string(activity_c.assets.large_text),
            },
            buttons,
        }
    }
}
fn rpc_set_activity(
    client: &mut DiscordIpcClient,
    activity: &ffi::Activity,
) -> Result<(), Box<dyn std::error::Error>> {
    warn!("State: {}, details: {}", &activity.state, &activity.details);
    let a = activity::Activity::new()
        .state(&activity.state)
        .details(&activity.details)
        .timestamps(activity::Timestamps::new().start(activity.timestamps.start))
        .assets(
            activity::Assets::new()
                .large_image(&activity.assets.large_image)
                .large_text(&activity.assets.large_text),
        )
        .buttons(
            activity
                .buttons
                .iter()
                .map(|x| activity::Button::new(&x.text, &x.link))
                .collect(),
        );
    client.set_activity(a)?;

    Ok(())
}

#[no_mangle]
pub extern "C" fn rsl_rpc_set_activity(client: &mut DiscordIpcClient, activity: &ActivityC) {
    trace!("[DiscordIpcClient] rsl_rpc_set_activity()...");
    let a = ffi::Activity::from(activity);
    let ok = rpc_set_activity(client, &a);
    match ok {
        Ok(_) => {
            trace!("[DiscordIpcClient] rsl_rpc_set_activity()...OK");
        }
        Err(err) => {
            let msg = err.to_string();
            error!("[DiscordIpcClient] {msg}");
        }
    };
}
