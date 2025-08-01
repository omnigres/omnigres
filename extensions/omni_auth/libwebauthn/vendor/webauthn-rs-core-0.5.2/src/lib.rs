//! Webauthn-rs - Webauthn for Rust Server Applications
//!
//! Webauthn is a standard allowing communication between servers, browsers and authenticators
//! to allow strong, passwordless, cryptographic authentication to be performed. Webauthn
//! is able to operate with many authenticator types, such as U2F.
//!
//! ⚠️  ⚠️  ⚠️  THIS IS UNSAFE. AVOID USING THIS DIRECTLY ⚠️  ⚠️  ⚠️
//!
//! If possible, use the `webauthn-rs` crate, and it's safe wrapper instead!
//!
//! Webauthn as a standard has many traps that in the worst cases, may lead to
//! bypasses and full account compromises. Many of the features of webauthn are
//! NOT security policy, but user interface hints. Many options can NOT be
//! enforced. `webauthn-rs` handles these correctly. USE `webauthn-rs` INSTEAD.

#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![deny(warnings)]
#![warn(unused_extern_crates)]
#![warn(missing_docs)]
#![deny(clippy::todo)]
#![deny(clippy::unimplemented)]
#![deny(clippy::unwrap_used)]
// #![deny(clippy::expect_used)]
#![deny(clippy::panic)]
#![deny(clippy::unreachable)]
#![deny(clippy::await_holding_lock)]
#![deny(clippy::needless_pass_by_value)]
#![deny(clippy::trivially_copy_pass_by_ref)]

#[macro_use]
extern crate tracing;

#[macro_use]
mod macros;

mod constants;

pub mod attestation;
pub mod crypto;
pub mod fake;

mod core;
pub mod error;
mod interface;
pub mod internals;

/// Protocol bindings
pub mod proto {
    pub use crate::interface::*;
    pub use base64urlsafedata::Base64UrlSafeData;
    pub use webauthn_rs_proto::*;
}

pub use crate::core::*;
