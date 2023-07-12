use std::process::Command;
use std::path::Path;

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
        (format!("{}{}", out_dir, "rsmeshopt"), "rsmeshopt"),
        (format!("{}{}", out_dir, "oishii"), "oishii"),
        (format!("{}{}", out_dir, "plate"), "plate"),
        (format!("{}{}", out_dir, "plugins"), "plugins"),
        (format!("{}{}", out_dir, "rsl"), "rsl"),
        (format!("{}{}", out_dir, "updater"), "updater"),
        (format!("{}{}", out_dir, "vendor"), "vendor"),
        (format!("{}{}", source_dir, "vendor"), "freetype"),
        (format!("{}{}", source_dir, "plate\\vendor\\glfw\\lib-vc2017"), "glfw3dll"),
        (format!("{}{}", source_dir, "vendor\\assimp"), "assimp-vc141-mt"),
        (format!("{}{}", deps_dir, "fmt-build"), "fmt"),
        (format!("{}{}", deps_dir, "libfort-build\\lib"), "fort"),
        (format!("{}{}", deps_dir, "meshoptimizer-build"), "meshoptimizer"),
    ];

    for (dir, lib) in libs {
        link_lib(&dir, &lib);
    }

    // gendef.exe llvm_sighandler.dll
    // llvm-dlltool.exe -D llvm_sighandler.dll -d llvm_sighandler.def -l llvm_sighandler.lib -m i386:x86-64

    let dlls = vec!["gctex.dll", "riistudio_rs.dll", "llvm_sighandler.dll"];

    #[cfg(windows)]
    for dll in &dlls {
        // Run gendef.exe
        let output = Command::new(format!("{}\\gendef.exe", cargo_dir))
            .arg(format!("{}\\{}", &cargo_dir, dll))
            .current_dir(&cargo_dir)
            .output()
            .expect("Failed to execute gendef.exe");

        // Check the output for any error
        if !output.status.success() {
            panic!("gendef.exe failed: {}", String::from_utf8_lossy(&output.stderr));
        }

        // Prepare .def and .lib file names
        let dll_path = Path::new(dll);
        let dll_stem = dll_path.file_stem().unwrap().to_str().unwrap();
        let def_file = format!("{}.def", dll_stem);
        let lib_file = format!("{}\\{}.lib", cargo_dir, dll_stem);

        // Run llvm-dlltool.exe
        let output = Command::new(format!("{}\\llvm-dlltool.exe", cargo_dir))
            .args(&["-D", dll, "-d", &def_file, "-l", &lib_file, "-m", "i386:x86-64"])
            .output()
            .expect("Failed to execute llvm-dlltool.exe");

        if !output.status.success() {
            panic!("llvm-dlltool.exe failed: {}", String::from_utf8_lossy(&output.stderr));
        }
    }

    println!("cargo:rustc-link-search=native={}", cargo_dir);
    println!("cargo:rustc-link-lib=dylib=gctex");
    println!("cargo:rustc-link-lib=dylib=riistudio_rs");
    println!("cargo:rustc-link-lib=dylib=llvm_sighandler");

    #[cfg(windows)]
    {
        println!("cargo:rustc-link-lib=static=opengl32");
        println!("cargo:rustc-link-lib=static=Crypt32");
        println!("cargo:rustc-link-lib=static=Secur32");
        println!("cargo:rustc-link-lib=static=Ncrypt");
        println!("cargo:rustc-link-lib=static=Shell32");
    }
}
