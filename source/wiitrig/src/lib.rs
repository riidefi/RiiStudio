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

mod c_api {
    #[no_mangle]
    pub unsafe extern "C" fn wii_sin(x: f32) -> f32 {
        crate::wii_sin(x)
    }

    #[no_mangle]
    pub unsafe extern "C" fn wii_cos(x: f32) -> f32 {
        crate::wii_cos(x)
    }
}

#[cfg(test)]
mod tests {
    use crate::*;

    #[test]
    fn test_wii_sin() {
        assert_eq!(wii_sin(0.0), 0.0);
    }

    #[test]
    fn test_wii_cos() {
        assert_eq!(wii_cos(0.0), 1.0);
    }
}
