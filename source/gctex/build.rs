extern crate bindgen;
extern crate cc;

use std::env;
use std::path::PathBuf;

fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);

    build.include(".").include("src");
    build.file("src/CmprEncoder.cpp");
    build.file("src/ImagePlatform.cpp");
    build.file("src/dolemu/MathUtil.cpp");
    build.file("src/dolemu/CPUDetect/GenericCPUDetect.cpp");
    build.file("src/dolemu/CPUDetect/x64CPUDetect.cpp");
    build.file("src/dolemu/TextureDecoder/TextureDecoder_Common.cpp");
    build.file("src/dolemu/TextureDecoder/TextureDecoder_Generic.cpp");
    build.file("src/dolemu/TextureDecoder/TextureDecoder_x64.cpp");
    build.file("src/bindings.cpp");
    build.compile("gctex.a");

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
