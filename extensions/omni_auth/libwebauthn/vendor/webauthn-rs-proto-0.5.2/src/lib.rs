//! JSON Protocol Structs and representations for communication with authenticators
//! and clients.

#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![deny(warnings)]
#![warn(unused_extern_crates)]
#![warn(missing_docs)]

pub mod attest;
pub mod auth;
pub mod cose;
pub mod extensions;
pub mod options;

#[cfg(feature = "wasm")]
pub mod wasm;

pub use attest::*;
pub use auth::*;
pub use cose::*;
pub use extensions::*;
pub use options::*;

#[cfg(feature = "wasm")]
pub use wasm::*;

#[cfg(feature = "wasm")]
use base64::engine::general_purpose::URL_SAFE_NO_PAD as BASE64_ENGINE;
