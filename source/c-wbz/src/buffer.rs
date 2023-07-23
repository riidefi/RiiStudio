use std::mem::ManuallyDrop;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Buffer {
    begin: *mut u8,
    len: usize,
}

impl Buffer {
    /// # Safety
    /// Buffer must have originally been created via the From<Vec<u8>> conversion.
    pub unsafe fn into_vec(self) -> Vec<u8> {
        unsafe { Vec::from_raw_parts(self.begin, self.len, self.len) }
    }
}

impl From<Vec<u8>> for Buffer {
    fn from(value: Vec<u8>) -> Self {
        let mut value = ManuallyDrop::new(value);
        value.shrink_to_fit();

        // Manual Vec::into_raw_parts
        let ptr = value.as_mut_ptr();
        let cap = value.capacity();
        let len = value.len();

        if cap != len {
            eprintln!("Failed to shrink {cap} to equal {len}!");
            std::process::abort();
        }

        Buffer { begin: ptr, len }
    }
}
