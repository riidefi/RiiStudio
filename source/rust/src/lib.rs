use simple_logger::SimpleLogger;
use std::ffi::CStr;
use std::os::raw::c_char;
use unzpack::Unzpack;

use discord_rich_presence::{activity, DiscordIpc, DiscordIpcClient};
use log::*;

/*
use std::fs::OpenOptions;
use std::os::windows::prelude::*;
*/

pub fn rsl_extract_zip(from_file: &str, to_folder: &str) -> Result<(), String> {
  // This fails for some reason with ERROR_SHARING_VIOLATION
  // Yet doing a simple fopen in C++ works..

  /*let outpath = "C:\\Users\\rii\\Documents\\dev\\RiiStudio\\out\\build\\Clang-x64-DIST\\source\\frontend\\assimp-vc141-mt.dll";
  OpenOptions::new()
          .write(true)
          .truncate(true)
          .access_mode(0x40000000)
          .share_mode(7)
          .open(&outpath).unwrap();
          */
  match Unzpack::extract(from_file, to_folder) {
    Ok(_) => Ok(()),
    Err(_) => Err("Failed to extract".to_string()),
  }
}

#[no_mangle]
pub unsafe fn c_rsl_extract_zip(from_file: *const c_char, to_folder: *const c_char) -> i32 {
  if from_file.is_null() || to_folder.is_null() {
    return -1;
  }
  let from_cstr = CStr::from_ptr(from_file);
  let to_cstr = CStr::from_ptr(to_folder);

  let from_str = String::from_utf8_lossy(from_cstr.to_bytes());
  let to_str = String::from_utf8_lossy(to_cstr.to_bytes());

  println!("Extracting zip (from {from_str} to {to_str})");

  match rsl_extract_zip(&from_str, &to_str) {
    Ok(_) => 0,
    Err(err) => {
      println!("Failed: {err}");
      -1
    }
  }
}

#[no_mangle]
pub fn rsl_log_init() {
  SimpleLogger::new().init().unwrap();
}

#[no_mangle]
pub unsafe fn rsl_c_debug(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  debug!("{}", &st);
}
#[no_mangle]
pub unsafe fn rsl_c_error(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  error!("{}", &st);
}
#[no_mangle]
pub unsafe fn rsl_c_info(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  info!("{}", &st);
}
/*
#[no_mangle]
pub unsafe fn rsl_c_log(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  log!("{}", &st);
}
*/
#[no_mangle]
pub unsafe fn rsl_c_trace(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  trace!("{}", &st);
}
#[no_mangle]
pub unsafe fn rsl_c_warn(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  warn!("{}", &st);
}

fn rpc_create(app_id: &str) -> Result<DiscordIpcClient, Box<dyn std::error::Error>> {
  warn!("[DiscordIpcClient] Creating client");
  DiscordIpcClient::new(app_id)
}
fn rpc_connect(client: &mut DiscordIpcClient) {
  warn!("[DiscordIpcClient] Connecting...");
  match client.connect() {
    Ok(_) => (),
    Err(err) => {
      let msg = err.to_string();
      error!("[DiscordIpcClient] {msg}");
    }
  }
}
fn rpc_kill(client: &mut DiscordIpcClient) {
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
pub extern "C" fn rsl_rpc_connect(client: &mut DiscordIpcClient) {
  warn!("[DiscordIpcClient] rsl_rpc_connect()");
  rpc_connect(client);
}

#[no_mangle]
pub extern "C" fn rsl_rpc_create(s: *const c_char) -> *mut DiscordIpcClient {
  let app_id = unsafe {
    String::from_utf8_lossy(CStr::from_ptr(s).to_bytes())
  };
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
    let mut boxed = Box::from_raw(client);
    rpc_kill(&mut *boxed);
  }
}
