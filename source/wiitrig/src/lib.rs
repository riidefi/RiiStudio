pub mod librii {
    #[allow(non_upper_case_globals)]
    #[allow(non_camel_case_types)]
    #[allow(non_snake_case)]
    pub mod bindings {
        include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
    }

    pub fn wii_sin(x: f32) -> f32 {
        unsafe { bindings::impl_wii_sin(x) }
    }

    pub fn wii_cos(x: f32) -> f32 {
        unsafe { bindings::impl_wii_cos(x) }
    }
}

#[no_mangle]
pub unsafe extern "C" fn wii_sin(x: f32) -> f32 {
    librii::wii_sin(x)
}

#[no_mangle]
pub unsafe extern "C" fn wii_cos(x: f32) -> f32 {
    librii::wii_cos(x)
}
