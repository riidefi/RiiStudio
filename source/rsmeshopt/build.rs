extern crate bindgen;
extern crate cc;

use std::env;
use std::path::PathBuf;

fn main() {
    // For version API
    println!(
        "cargo:rustc-env=TARGET={}",
        std::env::var("TARGET").unwrap()
    );
    let mut build = cc::Build::new();
    build.cpp(true);

    let compiler = build.get_compiler();
    let is_clang_cl =
        compiler.path().ends_with("clang-cl.exe") || compiler.path().ends_with("clang-cl");

    if is_clang_cl || compiler.is_like_msvc() {
        build.std("c++latest");
    } else {
        build.std("gnu++2b");
    }

    if compiler.is_like_clang() || is_clang_cl {
        // warning: src\draco/core/bounding_box.h:44:3: warning: 'const' type qualifier on return type has no effect [-Wignored-qualifiers]
        // warning:   const bool IsValid() const;
        build.flag("-Wno-ignored-qualifiers");
        // warning: src\draco/core/draco_index_type.h:150:25: warning: definition of implicit copy constructor for 'IndexType<unsigned int, draco::FaceIndex_tag_type_>' is deprecated because it has a user-provided copy assignment operator [-Wdeprecated-copy-with-user-provided-copy]
        // warning:   inline ThisIndexType &operator=(const ThisIndexType &i) {
        build.flag("-Wno-deprecated-copy-with-user-provided-copy");

        // warning: src/tristrip/trianglemesh.cpp:175:1: warning: non-void function does not return a value in all control paths [-Wreturn-type]
        // warning: };
        build.flag("-Wno-return-type");

        // warning: src/tristrip/trianglestripifier.cpp:67:4: warning: field 'strip_id' will be initialized after field 'experiment_id' [-Wreorder-ctor]
        // warning:           strip_id(TriangleStrip::NUM_STRIPS++),
        // warning:           ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // warning:           experiment_id(_experiment_id)
        build.flag("-Wno-reorder-ctor");

        // warning: src/TriStripper/tri_stripper.cpp:373:40: warning: unused typedef 'tri_node_iter' [-Wunused-local-typedef]
        // warning:         typedef triangle_graph::node_iterator tri_node_iter;
        build.flag("-Wno-unused-local-typedef");

        // My code
        // TODO: Fix
        build.flag("-Wno-sign-compare");
        build.flag("-Wno-unused-parameter");
    }

    if compiler.is_like_gnu() {
        // warning: src\draco/core/bounding_box.h:44:3: warning: 'const' type qualifier on return type has no effect [-Wignored-qualifiers]
        // warning:   const bool IsValid() const;
        build.flag("-Wno-ignored-qualifiers");

        // warning: src/TriStripper/tri_stripper.cpp:373:40: warning: unused typedef 'tri_node_iter' [-Wunused-local-typedef]
        // warning:         typedef triangle_graph::node_iterator tri_node_iter;
        build.flag("-Wno-unused-local-typedefs");

        // warning: src/tristrip/trianglestripifier.cpp:67:4: warning: field 'strip_id' will be initialized after field 'experiment_id' [-Wreorder-ctor]
        // warning:           strip_id(TriangleStrip::NUM_STRIPS++),
        // warning:           ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // warning:           experiment_id(_experiment_id)
        build.flag("-Wno-reorder");

        //   cargo:warning=src/draco/core/draco_index_type.h:150:25: note: because ‘draco::IndexType<unsigned int, draco::AttributeValueIndex_tag_type_>’ has user-provided ‘draco::IndexType<ValueTypeT, TagT>::ThisIndexType& draco::IndexType<ValueTypeT, TagT>::operator=(const ThisIndexType&) [with ValueTypeT = unsigned int; TagT = draco::AttributeValueIndex_tag_type_; draco::IndexType<ValueTypeT, TagT>::ThisIndexType = draco::IndexType<unsigned int, draco::AttributeValueIndex_tag_type_>]’
        //   cargo:warning=  150 |   inline ThisIndexType &operator=(const ThisIndexType &i) {
        //   cargo:warning=      |
        build.flag("-Wno-deprecated-copy");

        build.flag("-Wno-comment");
        // TODO: Fix
        build.flag("-Wno-sign-compare");

        // TODO: This is a workaround
        build.flag("-DRSL_USE_FALLBACK_EXPECTED=1");
        build.flag("-DRSL_STACKTRACE_UNSUPPORTED=1");
    }

    if !compiler.is_like_gnu() && !compiler.is_like_clang() {
        #[cfg(not(debug_assertions))]
        build.flag("-MT");
    }

    if compiler.is_like_gnu() || compiler.is_like_clang() || is_clang_cl {
        build.flag("-O2");
    }

    #[cfg(target_arch = "x86_64")]
    build.flag("-DARCH_X64=1");

    build.include(".").include("src");

    // Since MacOS has to use fmt, which we don't want to necessarily link
    #[cfg(target_os = "macos")]
    build.flag("-DRSL_EXPECT_NO_USE_FORMAT=1");

    build.file("src/bindings.cpp");
    build.file("src/HaroohieTriStripifier.cpp");
    build.file("src/RsMeshOpt.cpp");
    build.file("src/TriangleFanSplitter.cpp");

    build.file("src/draco/attributes/point_attribute.cc");
    build.file("src/draco/attributes/geometry_attribute.cc");
    build.file("src/draco/core/bounding_box.cc");
    build.file("src/draco/core/data_buffer.cc");
    build.file("src/draco/core/draco_types.cc");
    build.file("src/draco/mesh/corner_table.cc");
    build.file("src/draco/mesh/mesh.cc");
    build.file("src/draco/mesh/mesh_misc_functions.cc");
    build.file("src/draco/mesh/mesh_stripifier.cc");
    build.file("src/draco/point_cloud/point_cloud.cc");

    build.file("src/meshoptimizer/stripifier.cpp");

    build.file("src/tristrip/trianglemesh.cpp");
    build.file("src/tristrip/trianglestripifier.cpp");
    build.file("src/tristrip/tristrip.cpp");

    build.file("src/TriStripper/connectivity_graph.cpp");
    build.file("src/TriStripper/policy.cpp");
    build.file("src/TriStripper/tri_stripper.cpp");

    build.compile("rsmeshopt_rs.a");

    let bindings = bindgen::Builder::default()
        .header("src/bindings.h")
        .clang_arg("-fvisibility=default")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings");
}
