use simple_logger::SimpleLogger;
use std::ffi::CStr;
use std::os::raw::c_char;

use discord_rich_presence::{activity, DiscordIpc, DiscordIpcClient};
use log::*;

use std::path::PathBuf;
use std::io::Cursor;

use clap::CommandFactory;
use clap_derive::Parser;
use clap_derive::Subcommand;
use clap::Parser;
use std::os::raw::{c_int, c_uint, c_float};
use std::ffi::OsString;
use std::slice;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
pub struct MyArgs {
    #[command(subcommand)]
    pub command: Commands,
}

/// Import a .dae/.fbx file as .brres
#[derive(Parser, Debug)]
pub struct ImportCommand {
    /// File to import from: .dae or .fbx
    #[arg(required=true)]
    from: String,

    /// File to export to
    to: Option<String>,

    /// Scale to apply to the model
    #[arg(short, long, default_value = "1.0")]
    scale: f32,

    /// Whether to apply the Brawlbox scale fix
    #[clap(long)]
    brawlbox_scale: bool,

    /// Whether to generate mipmaps for textures
    #[clap(long, default_value="true")]
    mipmaps: bool,

    /// Minimum mipmap dimension to generate
    #[arg(long, default_value = "32")]
    min_mip: u32,

    /// Maximum number of mipmaps to generate
    #[arg(long, default_value = "5")]
    max_mip: u32,

    /// Whether to automatically set transparency
    #[clap(long, default_value="true")]
    auto_transparency: bool,

    /// Whether to merge materials with identical properties
    #[clap(long, default_value="true")]
    merge_mats: bool,

    /// Whether to bake UV transforms into vertices
    #[clap(long, default_value="false")]
    bake_uvs: bool,

    /// Tint to apply to the model in hex format (#RRGGBB)
    #[arg(long, default_value = "#FFFFFF")]
    tint: String,

    /// Whether to cull degenerate triangles
    #[clap(long, default_value="true")]
    cull_degenerates: bool,

    /// Whether to cull triangles with invalid vertices
    #[clap(long, default_value="true")]
    cull_invalid: bool,

    /// Whether to recompute normals
    #[clap(long, default_value="false")]
    recompute_normals: bool,

    /// Whether to fuse identical vertices
    #[clap(long, default_value="true")]
    fuse_vertices: bool,

    /// Disable triangle stripification
    #[clap(long, default_value="false")]
    no_tristrip: bool,

    /// 
    #[clap(long, default_value="false")]
    ai_json: bool,

    /// Read preset material/animation overrides from this folder
    #[clap(long)]
    preset_path: Option<String>,

    #[clap(short, long, default_value="false")]
    verbose: bool,
}

/// Decompress a .szs file
#[derive(Parser, Debug)]
pub struct DecompressCommand {
    /// File to decompress: *.szs
    #[arg(required=true)]
    from: String,

    /// Output file for decompressed file
    to: Option<String>,

    #[clap(short, long, default_value="false")]
    verbose: bool,
}

/// Compress a file as .szs
#[derive(Parser, Debug)]
pub struct CompressCommand {
    /// File to compress: *
    #[arg(required=true)]
    from: String,

    /// Output file for compressed file (.szs)
    to: Option<String>,

    #[clap(short, long, default_value="false")]
    verbose: bool,
}

#[derive(Subcommand, Debug)]
pub enum Commands {
    /// Import a .dae/.fbx file as .brres
    importCommand(ImportCommand),

    /// Decompress a .szs file
    Decompress(DecompressCommand),

    /// Compress a file as .szs
    Compress(CompressCommand),
}

#[repr(C)]
pub struct CliOptions {
    pub c_type: c_uint,

    // TYPE 1: "import-command"
    pub from: [c_char; 256],
    pub to: [c_char; 256],
    pub preset_path: [c_char; 256],
    pub scale: c_float,
    pub brawlbox_scale: c_uint,
    pub mipmaps: c_uint,
    pub min_mip: c_uint,
    pub max_mips: c_uint,
    pub auto_transparency: c_uint,
    pub merge_mats: c_uint,
    pub bake_uvs: c_uint,
    pub tint: c_uint,
    pub cull_degenerates: c_uint,
    pub cull_invalid: c_uint,
    pub recompute_normals: c_uint,
    pub fuse_vertices: c_uint,
    pub no_tristrip: c_uint,
    pub ai_json: c_uint,
    pub verbose: c_uint,

    // TYPE 2: "decompress"
    // Uses "from", "to" and "verbose" above
}

fn is_valid_hexcode(value: String) -> Result<(), String> {
    if value.len() != 7 {
        return Err("Hexcode must be 7 characters long".into());
    }
    if !value.starts_with("#") {
        return Err("Hexcode must start with '#'".into());
    }
    if !value[1..].chars().all(|c| c.is_ascii_hexdigit()) {
        return Err("Hexcode must be a valid hexadecimal number".into());
    }
    Ok(())
}
impl MyArgs {
    fn to_cli_options(&self) -> CliOptions {
        match &self.command {
            Commands::importCommand(i) => {
                let tint_val = u32::from_str_radix(&i.tint[1..], 16).unwrap_or(0xFF_FFFF);
                let mut from2 : [i8; 256]= [0; 256];
                let mut to2 : [i8; 256]= [0; 256];
                let mut preset_path2 : [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                let default_str2 = String::new();
                let preset_str_bytes = i.preset_path.as_ref().unwrap_or(&default_str2).as_bytes();
                from2[..from_bytes.len()].copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()].copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                preset_path2[..preset_str_bytes.len()].copy_from_slice(unsafe { &*(preset_str_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 1,
                    from: from2,
                    to: to2,
                    preset_path: preset_path2,
                    scale: i.scale as c_float,
                    brawlbox_scale: i.brawlbox_scale as c_uint,
                    mipmaps: i.mipmaps as c_uint,
                    min_mip: i.min_mip as c_uint,
                    max_mips: i.max_mip as c_uint,
                    auto_transparency: i.auto_transparency as c_uint,
                    merge_mats: i.merge_mats as c_uint,
                    bake_uvs: i.bake_uvs as c_uint,
                    tint: tint_val as c_uint,
                    cull_degenerates: i.cull_degenerates as c_uint,
                    cull_invalid: i.cull_invalid as c_uint,
                    recompute_normals: i.recompute_normals as c_uint,
                    fuse_vertices: i.fuse_vertices as c_uint,
                    no_tristrip: i.no_tristrip as c_uint,
                    ai_json: i.ai_json as c_uint,
                    verbose: i.verbose as c_uint,
                }
            },
            Commands::Decompress(i) => {
                let mut from2 : [i8; 256]= [0; 256];
                let mut to2 : [i8; 256]= [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()].copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()].copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 2,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path:  [0; 256],
                    scale: 0.0 as c_float,
                    brawlbox_scale: 0 as c_uint,
                    mipmaps: 0 as c_uint,
                    min_mip: 0 as c_uint,
                    max_mips: 0 as c_uint,
                    auto_transparency: 0 as c_uint,
                    merge_mats: 0 as c_uint,
                    bake_uvs: 0 as c_uint,
                    tint: 0 as c_uint,
                    cull_degenerates: 0 as c_uint,
                    cull_invalid: 0 as c_uint,
                    recompute_normals: 0 as c_uint,
                    fuse_vertices: 0 as c_uint,
                    no_tristrip: 0 as c_uint,
                    ai_json: 0 as c_uint,
                }
            },
            Commands::Compress(i) => {
                let mut from2 : [i8; 256]= [0; 256];
                let mut to2 : [i8; 256]= [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()].copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()].copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 3,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path:  [0; 256],
                    scale: 0.0 as c_float,
                    brawlbox_scale: 0 as c_uint,
                    mipmaps: 0 as c_uint,
                    min_mip: 0 as c_uint,
                    max_mips: 0 as c_uint,
                    auto_transparency: 0 as c_uint,
                    merge_mats: 0 as c_uint,
                    bake_uvs: 0 as c_uint,
                    tint: 0 as c_uint,
                    cull_degenerates: 0 as c_uint,
                    cull_invalid: 0 as c_uint,
                    recompute_normals: 0 as c_uint,
                    fuse_vertices: 0 as c_uint,
                    no_tristrip: 0 as c_uint,
                    ai_json: 0 as c_uint,
                }
            },
        }
    }
}

fn parse_args(argc: c_int, argv: *const *const c_char) -> Result<MyArgs, String> {
    let args: Vec<OsString> = unsafe { 
        slice::from_raw_parts(argv, argc as usize)
            .iter()
            .map(|&arg| {
                OsString::from(
                    String::from_utf8_lossy(
                        CStr::from_ptr(arg).to_bytes()
                    ).into_owned()
                )
            })
            .collect()
    };

    match MyArgs::try_parse_from(args) {
        Ok(args) => {
            match &args.command {
                Commands::importCommand(i) => {
                    let str = i.tint.to_string();
                    match is_valid_hexcode(str) {
                        Ok(_) => { () },
                        Err(e) => {
                            let mut cmd = MyArgs::command();
                            cmd.error(
                                clap::error::ErrorKind::ArgumentConflict,
                                &e,
                            );
                            println!("--tint: {}", e);
                            return Err("Bad hexcode".to_string())
                        }
                    }
                },
                _ => {},
            };
            Ok(args)
        },
        Err(e) => {
            println!("{}", e.to_string());
            Err("Bruh".to_string())
        }
    }
}

#[no_mangle]
pub extern "C" fn rs_parse_args(
    argc: c_int,
    argv: *const *const c_char,
    args: *mut CliOptions,
) -> c_int {
    match parse_args(argc, argv) {
        Ok(x) => {
            unsafe {
                std::ptr::write(args, x.to_cli_options());
            }
            0
        }
        Err(_) => {
            -1
        }
    }
}

/*
use std::fs::OpenOptions;
use std::os::windows::prelude::*;
*/

pub fn rsl_extract_zip(from_file: &str,
  to_folder: &str,
  write_file: unsafe extern "C" fn(*const c_char, *const u8, u32)
) -> Result<(), String> {
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
  let archive: Vec<u8> = std::fs::read(from_file).expect("Failed to read {from_file}");
  match zip_extract::extract(Cursor::new(&archive), &PathBuf::from(to_folder), false, write_file) {
    Ok(_) => Ok(()),
    Err(_) => Err("Failed to extract".to_string()),
  }
}

#[no_mangle]
pub unsafe fn c_rsl_extract_zip(
  from_file: *const c_char,
  to_folder: *const c_char,
  write_file: unsafe extern "C" fn(*const c_char, *const u8, u32)
) -> i32 {
  if from_file.is_null() || to_folder.is_null() {
    return -1;
  }
  let from_cstr = CStr::from_ptr(from_file);
  let to_cstr = CStr::from_ptr(to_folder);

  let from_str = String::from_utf8_lossy(from_cstr.to_bytes());
  let to_str = String::from_utf8_lossy(to_cstr.to_bytes());

  println!("Extracting zip (from {from_str} to {to_str})");

  match rsl_extract_zip(&from_str, &to_str, write_file) {
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
    let mut _owning_boxed = Box::from_raw(client);
  }
}
#[cxx::bridge(namespace = "rsl::ffi")]
mod ffi {
  struct Asset {
    large_image: String,
    large_text: String,
  }
  struct Button {
    text: String,
    link: String,
  }
  struct Timestamps {
    start: i64,
    end: i64,
  }
  struct Activity {
    state: String,
    details: String,
    timestamps: Timestamps,

    assets: Asset,
    buttons: Vec<Button>,
  }
}
fn rpc_set_activity(client: &mut DiscordIpcClient, activity: &ffi::Activity)
-> Result<(), Box<dyn std::error::Error>> {
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
    .buttons(activity.buttons.iter().map(|x| activity::Button::new(
      &x.text,
      &x.link,
    )).collect());
  client.set_activity(a)?;

  Ok(())
}
#[no_mangle]
pub extern "C" fn rsl_rpc_set_activity(client: &mut DiscordIpcClient,
                                       activity: &ffi::Activity) {
  trace!("[DiscordIpcClient] rsl_rpc_set_activity()...");
  let ok = rpc_set_activity(client, &activity);
  match ok {
    Ok(_) => {
      trace!("[DiscordIpcClient] rsl_rpc_set_activity()...OK");
    },
    Err(err) => {
      let msg = err.to_string();
      error!("[DiscordIpcClient] {msg}");
    }
  };
}
