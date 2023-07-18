# dolphin-memory-engine-rs
Rust bindings for https://github.com/aldelaro5/Dolphin-memory-engine

## Why?
Existing rust libraries do not support MEM2 or Linux. We can switch to those when feature parity is met.

## Example
```rs
use dolphin_memory_engine_rs::{DolphinAccessor, DolphinStatus};

fn main() {
    let mut dolphin_accessor = DolphinAccessor::instance();

    println!("Hooking dolphin_accessor");
    dolphin_accessor.hook();

    // Check the status of Dolphin
    let status = dolphin_accessor.get_status();
    match status {
        DolphinStatus::Hooked => println!("Dolphin is hooked."),
        DolphinStatus::NotRunning => println!("Dolphin is NotRunning."),
        DolphinStatus::NoEmu => println!("Dolphin is NoEmu."),
        DolphinStatus::UnHooked => println!("Dolphin is UnHooked."),
    }

    // Check if ARAM is accessible
    if dolphin_accessor.is_aram_accessible() {
        println!("ARAM is accessible.");
    } else {
        println!("ARAM is not accessible.");
    }

    // Read from RAM
    let mut buffer: [u8; 4] = [0; 4];
    let offset = 0x80000000; // Sample offset
    if dolphin_accessor.is_valid_console_address(offset) {
        println!("Address is valid.");
    } else {
        println!("Address is not valid.");
    }
    let result = dolphin_accessor.read_from_ram(offset, &mut buffer, false);

    if result {
        println!("Read from RAM successful. Data: {:?}", buffer);
        for &byte in buffer.iter() {
            println!("Character: {}", byte as char);
        }
    } else {
        println!("Read from RAM failed. Error code: {}", result);
    }

    // Write to RAM
    let data: [u8; 4] = [1, 2, 3, 4];
    let result = dolphin_accessor.write_to_ram(offset, &data, false);

    if result {
        println!("Write to RAM successful.");
    } else {
        println!("Write to RAM failed. Error code: {}", result);
    }

    // Always remember to unhook before ending the program
    dolphin_accessor.unhook();
}
```
