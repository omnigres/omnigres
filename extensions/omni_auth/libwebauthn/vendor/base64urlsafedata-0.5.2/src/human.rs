use std::fmt;

use crate::{Base64UrlSafeData, URL_SAFE_NO_PAD};
use base64::Engine;
use serde::{Serialize, Serializer};

/// Serde wrapper for `Vec<u8>` which emits URL-safe, non-padded Base64 for
/// *only* human-readable formats, and accepts Base64 and binary formats.
///
/// * Deserialisation is described in the [module documentation][crate].
///
/// * Serialisation to [a human-readable format][0] (such as JSON) emits
///   URL-safe, non-padded Base64 (per [RFC 4648 §5][sec5]).
///
/// * Serialisation to [a non-human-readable format][0] (such as CBOR) emits
///   a native "bytes" type, and not encode the value.
///
/// [0]: https://docs.rs/serde/latest/serde/trait.Serializer.html#method.is_human_readable
/// [sec5]: https://datatracker.ietf.org/doc/html/rfc4648#section-5
#[derive(Debug, Clone, PartialEq, Eq, Ord, PartialOrd, Hash)]
pub struct HumanBinaryData(Vec<u8>);

common_impls!(HumanBinaryData);

impl From<Base64UrlSafeData> for HumanBinaryData {
    fn from(value: Base64UrlSafeData) -> Self {
        Self(value.into())
    }
}

impl PartialEq<Base64UrlSafeData> for HumanBinaryData {
    fn eq(&self, other: &Base64UrlSafeData) -> bool {
        self.0.eq(other)
    }
}

impl Serialize for HumanBinaryData {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        if serializer.is_human_readable() {
            let encoded = URL_SAFE_NO_PAD.encode(self);
            serializer.serialize_str(&encoded)
        } else {
            serializer.serialize_bytes(self)
        }
    }
}
