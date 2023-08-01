use lazy_static::lazy_static;
use std::os::raw::c_char;
use std::sync::Mutex;

lazy_static! {
    pub static ref DOLPHIN_ACCESSOR: Mutex<DolphinAccessor> = Mutex::new(DolphinAccessor::new());
}

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
pub enum DolphinStatus {
    Hooked = 0,
    NotRunning = 1,
    NoEmu = 2,
    UnHooked = 3,
}

impl From<u32> for DolphinStatus {
    fn from(item: u32) -> Self {
        match item {
            0 => DolphinStatus::Hooked,
            1 => DolphinStatus::NotRunning,
            2 => DolphinStatus::NoEmu,
            3 => DolphinStatus::UnHooked,
            _ => panic!("Invalid value"),
        }
    }
}

pub struct DolphinAccessor;

impl Drop for DolphinAccessor {
    fn drop(&mut self) {
        unsafe { bindings::DolphinAccessor_free() };
    }
}

impl DolphinAccessor {
    // PRIVATE: The underlying library does not support multiple instances
    fn new() -> DolphinAccessor {
        unsafe { bindings::DolphinAccessor_init() };
        DolphinAccessor {}
    }

    pub fn instance() -> std::sync::MutexGuard<'static, DolphinAccessor> {
        DOLPHIN_ACCESSOR.lock().unwrap()
    }

    pub fn hook(&mut self) {
        unsafe { bindings::DolphinAccessor_hook() };
    }

    pub fn unhook(&mut self) {
        unsafe { bindings::DolphinAccessor_unHook() };
    }

    pub fn read_from_ram(&mut self, offset: u32, buffer: &mut [u8], with_b_swap: bool) -> bool {
        let size = buffer.len() as u32;
        let buffer_ptr = buffer.as_mut_ptr() as *mut c_char;

        let with_b_swap_i32 = if with_b_swap { 1 } else { 0 };

        let res = unsafe { bindings::DolphinAccessor_readFromRAM(offset, buffer_ptr, size, with_b_swap_i32) };
        match res {
            0 => false,
            _ => true,
        }
    }
    pub fn write_to_ram(&mut self, offset: u32, buffer: &[u8], with_b_swap: bool) -> bool {
        let size = buffer.len() as u32;
        let buffer_ptr = buffer.as_ptr() as *const c_char;

        let with_b_swap_i32 = if with_b_swap { 1 } else { 0 };

        let res = unsafe { bindings::DolphinAccessor_writeToRAM(offset, buffer_ptr, size, with_b_swap_i32) };
        match res {
            0 => false,
            _ => true,
        }
    }

    pub fn get_pid(&self) -> i32 {
        unsafe { bindings::DolphinAccessor_getPID() }
    }

    pub fn get_emu_ram_address_start(&self) -> u64 {
        unsafe { bindings::DolphinAccessor_getEmuRAMAddressStart() }
    }

    pub fn get_status(&self) -> DolphinStatus {
        let status = unsafe { bindings::DolphinAccessor_getStatus() } as u32;
        DolphinStatus::from(status)
    }

    pub fn is_aram_accessible(&self) -> bool {
        let res = unsafe { bindings::DolphinAccessor_isARAMAccessible() };
        match res {
            0 => false,
            _ => true,
        }
    }

    pub fn get_aram_address_start(&self) -> u64 {
        unsafe { bindings::DolphinAccessor_getARAMAddressStart() }
    }

    pub fn is_mem2_present(&self) -> bool {
        let res = unsafe { bindings::DolphinAccessor_isMEM2Present() };
        match res {
            0 => false,
            _ => true,
        }
    }

    pub fn is_valid_console_address(&self, address: u32) -> bool {
        let res = unsafe { bindings::DolphinAccessor_isValidConsoleAddress(address) };
        match res {
            0 => false,
            _ => true,
        }
    }
}

use std::os::raw::c_void;

#[repr(C)]
pub enum DolphinStatus_C {
    Hooked = 0,
    NotRunning = 1,
    NoEmu = 2,
    UnHooked = 3,
}

impl From<DolphinStatus> for DolphinStatus_C {
    fn from(status: DolphinStatus) -> Self {
        match status {
            DolphinStatus::Hooked => DolphinStatus_C::Hooked,
            DolphinStatus::NotRunning => DolphinStatus_C::NotRunning,
            DolphinStatus::NoEmu => DolphinStatus_C::NoEmu,
            DolphinStatus::UnHooked => DolphinStatus_C::UnHooked,
        }
    }
}

#[no_mangle]
pub extern "C" fn dolphin_accessor_new() -> *mut c_void {
    drop(DolphinAccessor::instance());
    100 as *mut c_void
}
#[no_mangle]
pub extern "C" fn dolphin_accessor_hook(_dummy: *mut c_void) {
  let mut dolphin_accessor = DolphinAccessor::instance();
    dolphin_accessor.hook();
}
#[no_mangle]
pub extern "C" fn dolphin_accessor_unhook(_dummy: *mut c_void) {
  let mut dolphin_accessor = DolphinAccessor::instance();
    dolphin_accessor.unhook();
}
#[no_mangle]
pub extern "C" fn dolphin_accessor_read_from_ram(
    _dummy: *mut c_void,
    offset: u32,
    buffer: *mut u8,
    buffer_length: u32,
    with_b_swap: bool,
) -> bool {
    let mut dolphin_accessor = DolphinAccessor::instance();
    let slice = unsafe { std::slice::from_raw_parts_mut(buffer, buffer_length as usize) };
    dolphin_accessor.read_from_ram(offset, slice, with_b_swap)
}

#[no_mangle]
pub extern "C" fn dolphin_accessor_write_to_ram(
    _dummy: *mut c_void,
    offset: u32,
    buffer: *const u8,
    buffer_length: u32,
    with_b_swap: bool,
) -> bool {
    let mut dolphin_accessor = DolphinAccessor::instance();
    let slice = unsafe { std::slice::from_raw_parts(buffer, buffer_length as usize) };
    dolphin_accessor.write_to_ram(offset, slice, with_b_swap)
}

#[no_mangle]
pub extern "C" fn dolphin_accessor_get_status(_dummy: *mut c_void) -> DolphinStatus_C {
    let dolphin_accessor = DolphinAccessor::instance();
    DolphinStatus_C::from(dolphin_accessor.get_status())
}
