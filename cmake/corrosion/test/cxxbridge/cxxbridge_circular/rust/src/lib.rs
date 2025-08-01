#[cxx::bridge]
pub mod ffi {
    #[derive(Debug, PartialEq)]
    pub struct Rgba {
        r: f32,
        g: f32,
        b: f32,
        a: f32,
    }

    #[derive(Debug, PartialEq)]
    pub struct RsImage {
        width: usize,
        height: usize,
        raster: Rgba,
    }
    unsafe extern "C++" {
        include!("cpplib.h");
        pub fn read_image(path: &str) -> RsImage;
    }

    extern "Rust" {
        pub fn equal_to(self: &RsImage, other: &str) -> bool;
    }
}

use ffi::*;

impl RsImage {
    pub fn equal_to(&self, path: &str) -> bool {
        println!("equal_to");
        *self == read_image(path)
    }
}
