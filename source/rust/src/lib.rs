use simple_logger::SimpleLogger;

use log::*;

use std::io::Cursor;
use std::path::PathBuf;

use clap::CommandFactory;
use clap::Parser;
use clap_derive::Parser;
use clap_derive::Subcommand;
use std::ffi::OsString;
use std::os::raw::{c_float, c_int, c_uint};
use std::slice;

use curl::easy::{Easy, WriteError};
use curl::Error;
use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::Write;
use std::os::raw::{c_char, c_double, c_void};
use std::path::Path;

pub fn download_string_rust(url: &str, user_agent: &str) -> Result<String, Error> {
    let mut easy = Easy::new();
    easy.url(url)?;
    easy.useragent(user_agent)?;

    let mut data = Vec::new();
    {
        let mut transfer = easy.transfer();
        transfer.write_function(|new_data| {
            data.extend_from_slice(new_data);
            Ok(new_data.len())
        })?;
        transfer.perform()?;
    }

    let result = String::from_utf8(data).unwrap();
    Ok(result)
}

pub fn download_file_rust(
    dest_path: &str,
    url: &str,
    user_agent: &str,
    progress_func: Box<ProgressFunc>,
    progress_data: *mut c_void,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut easy = Easy::new();
    easy.url(url)?;
    easy.useragent(user_agent)?;
    easy.follow_location(true)?;
    let path = Path::new(dest_path);
    let mut file = File::create(&path)
        .map_err(|err| format!("Couldn't create {}: {}", path.display(), err))?;

    let write_function = |data: &[u8]| -> Result<usize, WriteError> {
        file.write_all(data)
            .map_err(|_| WriteError::Pause)
            .map(|_| data.len())
    };
    let progress_function = |total: f64, current: f64, _, _| -> bool {
        progress_func(progress_data, total, current, 0.0, 0.0) == 0
    };
    let mut transfer = easy.transfer();
    transfer.write_function(write_function)?;
    transfer.progress_function(progress_function)?;
    transfer.perform().map_err(|err| err.into())
}

#[no_mangle]
pub extern "C" fn download_string(
    url: *const c_char,
    user_agent: *const c_char,
    err: *mut c_int,
) -> *mut c_char {
    let c_url = unsafe { CStr::from_ptr(url).to_str().unwrap() };
    let c_user_agent = unsafe { CStr::from_ptr(user_agent).to_str().unwrap() };

    match download_string_rust(c_url, c_user_agent) {
        Ok(result) => {
            unsafe {
                *err = 0;
            }
            CString::new(result).unwrap().into_raw()
        }
        Err(error) => {
            unsafe {
                *err = 1;
            }
            CString::new(error.to_string()).unwrap().into_raw()
        }
    }
}

pub type ProgressFunc = extern "C" fn(*mut c_void, c_double, c_double, c_double, c_double) -> c_int;

#[no_mangle]
pub extern "C" fn download_file(
    dest_path: *const c_char,
    url: *const c_char,
    user_agent: *const c_char,
    progress_func: ProgressFunc,
    progress_data: *mut c_void,
) -> *mut c_char {
    let c_dest_path = unsafe { CStr::from_ptr(dest_path).to_str().unwrap() };
    let c_url = unsafe { CStr::from_ptr(url).to_str().unwrap() };
    let c_user_agent = unsafe { CStr::from_ptr(user_agent).to_str().unwrap() };

    let result = download_file_rust(
        c_dest_path,
        c_url,
        c_user_agent,
        Box::new(progress_func),
        progress_data,
    );
    let response = match result {
        Ok(_) => String::from("Success"),
        Err(e) => e.to_string(),
    };

    CString::new(response).unwrap().into_raw()
}

#[no_mangle]
pub extern "C" fn free_string(s: *mut c_char) {
    unsafe {
        if s.is_null() {
            return;
        }
        let _ = CString::from_raw(s);
    };
}

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
    #[arg(required = true)]
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
    #[clap(long, default_value = "true")]
    mipmaps: bool,

    /// Minimum mipmap dimension to generate
    #[arg(long, default_value = "32")]
    min_mip: u32,

    /// Maximum number of mipmaps to generate
    #[arg(long, default_value = "5")]
    max_mip: u32,

    /// Whether to automatically set transparency
    #[clap(long, default_value = "true")]
    auto_transparency: bool,

    /// Whether to merge materials with identical properties
    #[clap(long, default_value = "false")]
    merge_mats: bool,

    /// Whether to bake UV transforms into vertices
    #[clap(long, default_value = "false")]
    bake_uvs: bool,

    /// Tint to apply to the model in hex format (#RRGGBB)
    #[arg(long, default_value = "#FFFFFF")]
    tint: String,

    /// Whether to cull degenerate triangles
    #[clap(long, default_value = "true")]
    cull_degenerates: bool,

    /// Whether to cull triangles with invalid vertices
    #[clap(long, default_value = "true")]
    cull_invalid: bool,

    /// Whether to recompute normals
    #[clap(long, default_value = "false")]
    recompute_normals: bool,

    /// Whether to fuse identical vertices
    #[clap(long, default_value = "true")]
    fuse_vertices: bool,

    /// Disable triangle stripification
    #[clap(long, default_value = "false")]
    no_tristrip: bool,

    ///
    #[clap(long, default_value = "false")]
    ai_json: bool,

    /// Do not SZS-compress the archive
    #[clap(short, long, default_value = "false")]
    no_compression: bool,

    /// Read preset material/animation overrides from this folder
    #[clap(long)]
    preset_path: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Decompress a .szs file
#[derive(Parser, Debug)]
pub struct DecompressCommand {
    /// File to decompress: *.szs
    #[arg(required = true)]
    from: String,

    /// Output file for decompressed file
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Compress a file as .szs
#[derive(Parser, Debug)]
pub struct CompressCommand {
    /// File to compress: *
    #[arg(required = true)]
    from: String,

    /// Output file for compressed file (.szs)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Convert a .rhst file to a .brres file
#[derive(Parser, Debug)]
pub struct Rhst2BrresCommand {
    /// RHST .json file to read
    #[arg(required = true)]
    from: String,

    /// Output .brres file
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Convert a .rhst file to a .bmd file
#[derive(Parser, Debug)]
pub struct Rhst2BmdCommand {
    /// RHST .json file to read
    #[arg(required = true)]
    from: String,

    /// Output .bmd file
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Extract a possibly SZS-compressed archive to a folder.
#[derive(Parser, Debug)]
pub struct ExtractCommand {
    /// SZS-compressed archive to read (accepts U8 or RARC)
    #[arg(required = true)]
    from: String,

    /// Output folder (or none for default)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Create a an archive from a folder.
#[derive(Parser, Debug)]
pub struct CreateCommand {
    /// Input folder
    #[arg(required = true)]
    from: String,

    /// Archive to write (or none for default)
    to: Option<String>,

    /// Do not SZS-compress the archive
    #[clap(short, long, default_value = "false")]
    no_compression: bool,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

#[derive(Subcommand, Debug)]
pub enum Commands {
    /// Import a .dae/.fbx file as .brres
    ImportCommand(ImportCommand),

    /// Decompress a .szs file
    Decompress(DecompressCommand),

    /// Compress a file as .szs
    Compress(CompressCommand),

    /// Convert a .rhst file to a .brres file
    Rhst2Brres(Rhst2BrresCommand),

    /// Convert a .rhst file to a .bmd file
    Rhst2Bmd(Rhst2BmdCommand),

    /// Extract a .szs file to a folder.
    Extract(ExtractCommand),

    /// Create a .szs file from a folder.
    Create(CreateCommand),
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
    pub no_compression: c_uint,
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
            Commands::ImportCommand(i) => {
                let tint_val = u32::from_str_radix(&i.tint[1..], 16).unwrap_or(0xFF_FFFF);
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let mut preset_path2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                let default_str2 = String::new();
                let preset_str_bytes = i.preset_path.as_ref().unwrap_or(&default_str2).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                preset_path2[..preset_str_bytes.len()]
                    .copy_from_slice(unsafe { &*(preset_str_bytes as *const _ as *const [i8]) });
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
                    no_compression: i.no_compression as c_uint,
                    verbose: i.verbose as c_uint,
                }
            }
            Commands::Decompress(i) => {
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 2,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path: [0; 256],
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
                    no_compression: 0 as c_uint,
                }
            }
            Commands::Compress(i) => {
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 3,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path: [0; 256],
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
                    no_compression: 0 as c_uint,
                }
            }
            Commands::Rhst2Brres(i) => {
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 4,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path: [0; 256],
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
                    no_compression: 0 as c_uint,
                }
            }
            Commands::Rhst2Bmd(i) => {
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 5,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path: [0; 256],
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
                    no_compression: 0 as c_uint,
                }
            }
            Commands::Extract(i) => {
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 6,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    preset_path: [0; 256],
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
                    no_compression: 0 as c_uint,
                }
            }
            Commands::Create(i) => {
                let mut from2: [i8; 256] = [0; 256];
                let mut to2: [i8; 256] = [0; 256];
                let from_bytes = i.from.as_bytes();
                let default_str = String::new();
                let to_bytes = i.to.as_ref().unwrap_or(&default_str).as_bytes();
                from2[..from_bytes.len()]
                    .copy_from_slice(unsafe { &*(from_bytes as *const _ as *const [i8]) });
                to2[..to_bytes.len()]
                    .copy_from_slice(unsafe { &*(to_bytes as *const _ as *const [i8]) });
                CliOptions {
                    c_type: 7,
                    from: from2,
                    to: to2,
                    verbose: i.verbose as c_uint,
                    no_compression: i.no_compression as c_uint,

                    // Junk fields
                    preset_path: [0; 256],
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
            }
        }
    }
}

fn parse_args(argc: c_int, argv: *const *const c_char) -> Result<MyArgs, String> {
    let args: Vec<OsString> = unsafe {
        slice::from_raw_parts(argv, argc as usize)
            .iter()
            .map(|&arg| {
                OsString::from(String::from_utf8_lossy(CStr::from_ptr(arg).to_bytes()).into_owned())
            })
            .collect()
    };

    match MyArgs::try_parse_from(args) {
        Ok(args) => {
            match &args.command {
                Commands::ImportCommand(i) => {
                    let str = i.tint.to_string();
                    match is_valid_hexcode(str) {
                        Ok(_) => (),
                        Err(e) => {
                            let mut cmd = MyArgs::command();
                            cmd.error(clap::error::ErrorKind::ArgumentConflict, &e);
                            println!("--tint: {}", e);
                            return Err("Bad hexcode".to_string());
                        }
                    }
                }
                _ => {}
            };
            Ok(args)
        }
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
        Err(_) => -1,
    }
}

pub fn rsl_extract_zip(from_file: &str, to_folder: &str) -> Result<(), String> {
    let archive: Vec<u8> = std::fs::read(from_file).expect("Failed to read {from_file}");
    match zip_extract::extract(Cursor::new(&archive), &PathBuf::from(to_folder), false) {
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
