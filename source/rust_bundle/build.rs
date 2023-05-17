
fn link_lib(directory: &str, lib: &str) {
    println!("cargo:rustc-link-search=native={}", directory);
    println!("cargo:rustc-link-lib=static={}", lib);
}

fn main() {
    let build_dir = "..\\..\\out\\build\\Clang-x64-DIST\\";
    let source_dir = "..\\..\\source\\";
    let out_dir = format!("{}source\\", build_dir);
    let deps_dir = format!("{}_deps\\", build_dir);
    let cargo_dir = format!("{}cargo\\build\\x86_64-pc-windows-msvc\\release\\", build_dir);

    let libs = vec![
        (format!("{}{}", out_dir, "frontend"), "lib_frontend"),
        (format!("{}{}", out_dir, "core"), "core"),
        (format!("{}{}", out_dir, "LibBadUIFramework"), "LibBadUIFramework"),
        (format!("{}{}", out_dir, "librii"), "librii"),
        (format!("{}{}", out_dir, "oishii"), "oishii"),
        (format!("{}{}", out_dir, "plate"), "plate"),
        (format!("{}{}", out_dir, "plugins"), "plugins"),
        (format!("{}{}", out_dir, "rsl"), "rsl"),
        (format!("{}{}", out_dir, "updater"), "updater"),
        (format!("{}{}", out_dir, "vendor"), "vendor"),
        (format!("{}{}", source_dir, "vendor"), "freetype"),
        (format!("{}{}", source_dir, "vendor"), "libcurl"),
        (format!("{}{}", source_dir, "plate\\vendor\\glfw\\lib-vc2017"), "glfw3dll"),
        (format!("{}{}", source_dir, "vendor\\assimp"), "assimp-vc141-mt"),
        (format!("{}{}", deps_dir, "fmt-build"), "fmt"),
        (format!("{}{}", deps_dir, "draco-build"), "draco"),
        (format!("{}{}", deps_dir, "libfort-build\\lib"), "fort"),
        (format!("{}{}", deps_dir, "meshoptimizer-build"), "meshoptimizer"),
        (cargo_dir.to_string(), "riistudio_rs")
    ];

    for (dir, lib) in libs {
        link_lib(&dir, &lib);
    }

    println!("cargo:rustc-link-lib=static=opengl32");
}
