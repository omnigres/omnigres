//! Wrappers for `Vec<u8>` to make Serde serialise and deserialise as URL-safe,
//! non-padded Base64 (per [RFC 4648 §5][sec5]).
//!
//! ## Serialisation behaviour
//!
//! * [`Base64UrlSafeData`] always serialises to URL-safe, non-padded Base64.
//!
//! * [`HumanBinaryData`] only serialises to URL-safe, non-padded Base64 when
//!   using a [human-readable format][0].
//!
//!   Otherwise, it serialises as a "bytes"-like type (like [`serde_bytes`][1]).
//!
//!   This feature is new in `base64urlsafe` v0.1.4.
//!
//! By comparison, Serde's default behaviour is to serialise `Vec<u8>` as a
//! sequence of integers. This is a problem for many formats:
//!
//! * `serde_cbor` encodes as an `array`, rather than a `bytes`. This uses
//!   zig-zag encoded integers for values > `0x1F`, which averages about 1.88
//!   bytes per byte assuming an equal distribution of values.
//!
//! * `serde_json` encodes as an `Array<Number>`, which averages 3.55 bytes per
//!   byte without whitespace.
//!
//! Using Base64 encoding averages 1.33 bytes per byte, and most formats pass
//! strings nearly-verbatim.
//!
//! ## Deserialisation behaviour
//!
//! Both types will deserialise multiple formats, provided the format is
//! self-describing (ie: [implements `deserialize_any`][5]):
//!
//! * Bytes types are passed as-is (new in v0.1.4).
//!
//!   [`HumanBinaryData`] produces this for [non-human-readable formats][0].
//!
//! * Sequences of integers are passed as-is.
//!
//!   Serde's default `Vec<u8>` serialiser produces this for many formats.
//!
//! * Strings are decoded Base64 per [RFC 4648 §5 (URL-safe)][sec5] or
//!   [§4 (standard)][sec4], with optional padding.
//!
//!   [`Base64UrlSafeData`] produces this for all formats, and
//!   [`HumanBinaryData`] produces this for [human-readable formats][0]. This
//!   should also be compatible with many other serialisers.
//!
//! ## Migrating from `Base64UrlSafeData` to `HumanBinaryData`
//!
//! [`Base64UrlSafeData`] always uses Base64 encoding, which isn't optimal for
//! many binary formats. For that reason, it's a good idea to migrate to
//! [`HumanBinaryData`] if you're using a binary format.
//!
//! However, you'll need to make sure *all* readers using [`Base64UrlSafeData`]
//! are on `base64urlsafedata` v0.1.4 or later before switching *anything* to
//! [`HumanBinaryData`]. Otherwise, they'll not be able to read any data in the
//! new format!
//!
//! Once they're all migrated across, you can start issuing writes in the new
//! format. It's a good idea to slowly roll out the change, in case you discover
//! something has been left behind.
//!
//! ## Alternatives
//!
//! * [`serde_bytes`][1], which implements efficient coding of `Vec<u8>`
//!   [for non-human-readable formats only][2].
//!
//! [0]: https://docs.rs/serde/latest/serde/trait.Serializer.html#method.is_human_readable
//! [1]: https://docs.rs/serde_bytes
//! [2]: https://github.com/serde-rs/bytes/issues/37
//! [5]: https://serde.rs/impl-deserialize.html
//! [sec4]: https://datatracker.ietf.org/doc/html/rfc4648#section-4
//! [sec5]: https://datatracker.ietf.org/doc/html/rfc4648#section-5
#![deny(warnings)]
#![warn(unused_extern_crates)]
#![deny(clippy::todo)]
#![deny(clippy::unimplemented)]
#![deny(clippy::unwrap_used)]
#![deny(clippy::expect_used)]
#![deny(clippy::panic)]
#![deny(clippy::unreachable)]
#![deny(clippy::await_holding_lock)]
#![deny(clippy::needless_pass_by_value)]
#![deny(clippy::trivially_copy_pass_by_ref)]

#[macro_use]
mod common;
mod human;
#[cfg(test)]
mod tests;

pub use crate::human::HumanBinaryData;

use base64::{
    engine::general_purpose::{
        GeneralPurpose, STANDARD, STANDARD_NO_PAD, URL_SAFE, URL_SAFE_NO_PAD,
    },
    Engine,
};
use serde::{Serialize, Serializer};
use std::fmt;
use std::hash::Hash;

static ALLOWED_DECODING_FORMATS: &[GeneralPurpose] =
    &[URL_SAFE_NO_PAD, URL_SAFE, STANDARD, STANDARD_NO_PAD];

/// Serde wrapper for `Vec<u8>` which always emits URL-safe, non-padded Base64,
/// and accepts Base64 and binary formats.
///
/// * Deserialisation is described in the [module documentation][crate].
///
/// * Serialisation *always* emits URL-safe, non-padded Base64 (per
///   [RFC 4648 §5][sec5]).
///
///   Unlike [`HumanBinaryData`], this happens *regardless* of whether the
///   underlying serialisation format is [human readable][0]. If you're
///   serialising to [non-human-readable formats][0], you should consider
///   [migrating to `HumanBinaryData`][crate].
///
/// Otherwise, this type should work as much like a `Vec<u8>` as possible.
///
/// [0]: https://docs.rs/serde/latest/serde/trait.Serializer.html#method.is_human_readable
/// [sec5]: https://datatracker.ietf.org/doc/html/rfc4648#section-5
#[derive(Debug, Clone, PartialEq, Eq, Ord, PartialOrd, Hash)]
pub struct Base64UrlSafeData(Vec<u8>);

common_impls!(Base64UrlSafeData);

impl From<HumanBinaryData> for Base64UrlSafeData {
    fn from(value: HumanBinaryData) -> Self {
        Self(value.into())
    }
}

impl PartialEq<HumanBinaryData> for Base64UrlSafeData {
    fn eq(&self, other: &HumanBinaryData) -> bool {
        self.0.eq(other)
    }
}

impl Serialize for Base64UrlSafeData {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let encoded = URL_SAFE_NO_PAD.encode(self);
        serializer.serialize_str(&encoded)
    }
}
