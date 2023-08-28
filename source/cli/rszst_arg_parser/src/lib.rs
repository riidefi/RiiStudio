use clap::{CommandFactory, Parser};
use clap_derive::Parser;
use clap_derive::Subcommand;
use clap_derive::ValueEnum;

extern crate colored;
use colored::*;

use core::ffi::c_char;
use core::ffi::c_float;
use core::ffi::c_int;
use core::ffi::c_uint;
use core::ffi::CStr;
use core::slice;
use std::ffi::OsString;

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

/// [Deep cut, debug] For generating KCL testing suite data, we dump "Jmp2Bmd" files to
/// .obj to recover original triangle data -- kcl files, on their own, are quite lossy.
#[derive(Parser, Debug)]
pub struct PreciseBMDDump {
    /// File to dump: *.bmd
    #[arg(required = true)]
    from: String,

    /// Output file for decompressed file
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

// Sync with librii::szs
#[repr(u32)]
#[derive(Debug, Copy, Clone, ValueEnum)]
enum SzsAlgo {
    WorstCaseEncoding,
    Nintendo,
    MkwSp,
    CTGP,
    Haroohie,
    CTLib,
}

/// Compress a file as .szs
#[derive(Parser, Debug)]
pub struct CompressCommand {
    /// File to compress: *
    #[arg(required = true)]
    from: String,

    /// Output file for compressed file (.szs)
    to: Option<String>,

    /// Compression algorithm to use.
    #[clap(short, long, value_enum)]
    algorithm: Option<SzsAlgo>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Dump a kmp as json
#[derive(Parser, Debug)]
pub struct KmpToJson {
    /// File to dump (.kmp)
    #[arg(required = true)]
    from: String,

    /// Output file for json (.json)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Convert to kmp from json
#[derive(Parser, Debug)]
pub struct JsonToKmp {
    /// File to read (.json)
    #[arg(required = true)]
    from: String,

    /// Output file for kmp (.kmp)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Dump a kcl as json
#[derive(Parser, Debug)]
pub struct KclToJson {
    /// File to dump (.kcl)
    #[arg(required = true)]
    from: String,

    /// Output file for json (.json)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Convert to kcl from json
#[derive(Parser, Debug)]
pub struct JsonToKcl {
    /// File to read (.json)
    #[arg(required = true)]
    from: String,

    /// Output file for kcl (.kcl)
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

    /// Compile the archive as RARC (GC) instead of U8 (Wii)
    #[clap(long, default_value = "false")]
    rarc: bool,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Dump presets of a model to a folder.
#[derive(Parser, Debug)]
pub struct DumpPresets {
    /// BRRES archive to read
    #[arg(required = true)]
    from: String,

    /// Output folder (or none for default)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

/// Optimize a BRRES or BMD file.
#[derive(Parser, Debug)]
pub struct Optimize {
    /// BRRES archive to read
    #[arg(required = true)]
    from: String,

    /// BRRES archive to write (or none for default)
    to: Option<String>,

    #[clap(short, long, default_value = "false")]
    verbose: bool,
}

#[derive(Subcommand, Debug)]
pub enum Commands {
    /// DEPRECATED: Use import-brres
    ImportCommand(ImportCommand),

    /// Import a .dae/.fbx file as .brres
    ImportBrres(ImportCommand),

    /// Import a .dae/.fbx file as .bmd
    ImportBmd(ImportCommand),

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

    KmpToJson(KmpToJson),
    JsonToKmp(JsonToKmp),

    KclToJson(KclToJson),
    JsonToKcl(JsonToKcl),

    DumpPresets(DumpPresets),

    PreciseBMDDump(PreciseBMDDump),

    Optimize(Optimize),
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
    pub rarc: c_uint,
    pub verbose: c_uint,
    pub szs_algo: c_uint,
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
                eprintln!("{}", "\n|--------------------------------\n|\n| Warning: `import-command` has been deprecated in favor of `import-brres`!\n|\n|--------------------------------\n".red().bold());
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
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    no_compression: 0 as c_uint,
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::ImportBrres(i) => {
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
                    verbose: i.verbose as c_uint,

                    // Junk fields
                    no_compression: 0 as c_uint,
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::ImportBmd(i) => {
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
                    c_type: 12,
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

                    // Junk fields
                    no_compression: 0 as c_uint,
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
                    szs_algo: i.algorithm.unwrap_or(SzsAlgo::CTGP) as c_uint,

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
                    rarc: 0 as c_uint,
                }
            }
            Commands::KmpToJson(i) => {
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
                    c_type: 8,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::JsonToKmp(i) => {
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
                    c_type: 9,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::KclToJson(i) => {
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
                    c_type: 10,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::JsonToKcl(i) => {
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
                    c_type: 11,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
                    rarc: i.rarc as c_uint,

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
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::DumpPresets(i) => {
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
                    c_type: 13,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::PreciseBMDDump(i) => {
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
                    c_type: 14,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
                }
            }
            Commands::Optimize(i) => {
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
                    c_type: 15,
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
                    rarc: 0 as c_uint,
                    szs_algo: 0 as c_uint,
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
