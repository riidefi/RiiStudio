#[cfg(feature = "run_bindgen")]
extern crate bindgen;
#[cfg(feature = "cpp_fallback")]
extern crate cc;

#[cfg(feature = "run_bindgen")]
use std::env;
#[cfg(feature = "run_bindgen")]
use std::path::PathBuf;

use glob;
use std::path::PathBuf;

#[cfg(windows)]
const SEARCH_WINDOWS: &[&str] = &[
    "C:\\LLVM\\bin",
    "C:\\Program Files*\\LLVM\\bin",
    "C:\\MSYS*\\MinGW*\\bin",
];

#[cfg(windows)]
fn find_clang() -> Option<PathBuf> {
    for &pattern in SEARCH_WINDOWS {
        if let Some(path) = find_in_pattern(pattern, "clang.exe") {
            return Some(path);
        }
    }
    None
}

#[cfg(windows)]
fn find_in_pattern(pattern: &str, filename: &str) -> Option<PathBuf> {
    let glob_pattern = format!("{}\\{}", pattern, filename);
    for entry in glob::glob(&glob_pattern).expect("Failed to read glob pattern") {
        if let Ok(path) = entry {
            return Some(path);
        }
    }
    None
}

fn main() {
    println!(
        "cargo:rustc-env=TARGET={}",
        std::env::var("TARGET").unwrap()
    );
    let target = std::env::var("TARGET").unwrap();
    #[cfg(feature = "cpp_fallback")]
    {
        let mut build = cc::Build::new();

        // AppleClang/GCC are fine. Just MSVC is problematic.

        #[cfg(windows)]
        match find_clang() {
            Some(path) => println!("Found clang at: {}", path.display()),
            None => panic!("Clang not found"),
        }
        #[cfg(windows)]
        build.compiler(find_clang().unwrap());

        build.cpp(true);

        build.std("c++23");

        if target.starts_with("x86_64-") {
            build.flag("-DARCH_X64=1");
        }

        let compiler = build.get_compiler();
        let is_clang_cl =
            compiler.path().ends_with("clang-cl.exe") || compiler.path().ends_with("clang-cl");
        if compiler.is_like_gnu() || compiler.is_like_clang() || is_clang_cl {
            if target.starts_with("x86_64-") {
                build.flag("-mssse3");
            }
        }
        if compiler.is_like_clang() || is_clang_cl {
            // warning : src/dolemu/TextureDecoder/TextureDecoder_x64.cpp(1177,75): warning: unused parameter 'tlut' [-Wunused-parameter]
            // warning : TextureFormat texformat, const u8* tlut, TLUTFormat tlutfmt,
            // warning : ^
            build.flag("-Wno-unused-parameter");

            build.flag("-Wno-sign-compare");
            build.flag("-Wno-unused-variable");
            build.flag("-Wno-deprecated-copy");
        }
        if !compiler.is_like_gnu() && !compiler.is_like_clang() {
            #[cfg(not(debug_assertions))]
            build.flag("-MT");
        }

        build.include(".").include("src");
        build.file("src/bindings.cpp");

        build.define("rii_compute_image_size", "rii_compute_image_size2");

        build.define("rsl_log_init", "rsl_log_init2");
        build.define("rsl_c_debug", "rsl_c_debug2");
        build.define("rsl_c_error", "rsl_c_error2");
        build.define("rsl_c_info", "rsl_c_info2");
        build.define("rsl_c_warn", "rsl_c_warn2");
        build.define("rsl_c_trace", "rsl_c_trace2");

        build.define("wii_sin", "wii_sin2");
        build.define("wii_cos", "wii_cos2");

        build
            .include(".")
            .include("../librii/g3d")
            .include("../")
            .include("../vendor")
            .include("../plate/include")
            .include("../plate/vendor")
            .file("../librii/g3d/io/TextureIO.cpp")
            .file("../librii/g3d/io/AnimIO.cpp")
            .file("../librii/g3d/io/TevIO.cpp")
            .file("../librii/g3d/io/NameTableIO.cpp")
            .file("../librii/g3d/io/DictWriteIO.cpp")
            .file("../librii/g3d/io/MatIO.cpp")
            .file("../librii/g3d/io/BoneIO.cpp")
            .file("../librii/g3d/io/ModelIO.cpp")
            .file("../librii/g3d/io/ArchiveIO.cpp")
            .file("../librii/g3d/io/AnimTexPatIO.cpp")
            .file("../librii/g3d/io/AnimClrIO.cpp")
            .file("../librii/g3d/io/AnimVisIO.cpp")
            .file("../librii/g3d/io/PolygonIO.cpp")
            .file("../librii/g3d/io/AnimChrIO.cpp")
            .file("../librii/g3d/io/JSON.cpp")
            .file("../librii/g3d/io/G3dJson.cpp");

        build
            .file("../rsl/SafeReader.cpp")
            .file("../rsl/WriteFile.cpp")
            .file("../rsl/Log.cpp")
            .file("../oishii/AbstractStream.cxx")
            .file("../oishii/BreakpointHolder.cxx")
            .file("../oishii/writer/binary_writer.cxx")
            .file("../oishii/writer/linker.cxx")
            .file("../oishii/writer/node.cxx")
            .file("../oishii/util/util.cxx")
            .file("../oishii/reader/binary_reader.cxx")
            .file("../librii/gx/Texture.cpp")
            .file("../librii/gpu/DLInterpreter.cpp")
            .file("../librii/gpu/DLPixShader.cpp")
            .file("../librii/gpu/GPUMaterial.cpp")
            .file("../vendor/fmt/format.cc")
            .file("../vendor/fmt/os.cc")
            .file("../librii/trig/WiiTrig.cpp");

        build.compile("brres.a");
    }
    #[cfg(feature = "run_bindgen")]
    {
        let bindings = bindgen::Builder::default()
            .header("src/bindings.h")
            .parse_callbacks(Box::new(bindgen::CargoCallbacks))
            .generate()
            .expect("Unable to generate bindings");

        let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
        bindings
            .write_to_file(out_path.join("bindings.rs"))
            .expect("Couldn't write bindings");
    }
}
