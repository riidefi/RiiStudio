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
