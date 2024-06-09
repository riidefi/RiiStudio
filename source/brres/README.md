# brres
WIP Rust/C++ crate for reading BRRES files

## Examples
```rs
fn test_read_raw_brres() {
    // Read the sea.brres file into a byte array
    let brres_data = fs::read("sea.brres").expect("Failed to read sea.brres file");

    // Call the read_raw_brres function
    match read_raw_brres(&brres_data) {
        Ok(archive) => {
            println!("{:#?}", archive.get_model("sea").unwrap().meshes[0]);
        }
        Err(e) => {
            panic!("Error reading brres file: {:#?}", e);
        }
    }
}
```

## Progress
Implements a Rust layer on top of `librii::g3d`'s JSON export-import layer. Importantly, large buffers like texture data and vertex data are not actually encoded in JSON but passed directly as a binary blob. This allows JSON files to stay light and results in minimal encoding latency (tests TBD).

| Format | Supported |
|--------|-----------|
| MDL0   | Yes       |
| TEX0   | Yes       |
| SRT0   | Yes       |
| PAT0   | No        |
| CLR0   | No        |
| CHR0   | Yes       |
| VIS0   | No        |


## Tests
Unit tests are being used to validate correctness. Run the suite with `cargo test`

## `brres-sys`
Low level documentation available [here](lib/brres-sys/README.md).
