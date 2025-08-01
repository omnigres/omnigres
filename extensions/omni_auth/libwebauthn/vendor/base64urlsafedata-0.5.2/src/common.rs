/// Macro to declare common functionality for [`Base64UrlSafeData`][0] and
/// [`HumanBinaryData`][1]
///
/// [0]: crate::Base64UrlSafeData
/// [1]: crate::HumanBinaryData
macro_rules! common_impls {
    ($type:ty) => {
        impl $type {
            pub const fn new() -> Self {
                Self(Vec::new())
            }

            pub fn with_capacity(capacity: usize) -> Self {
                Vec::with_capacity(capacity).into()
            }
        }

        impl Default for $type {
            fn default() -> Self {
                Self::new()
            }
        }

        impl std::ops::Deref for $type {
            type Target = Vec<u8>;

            fn deref(&self) -> &Self::Target {
                &self.0
            }
        }

        impl std::ops::DerefMut for $type {
            fn deref_mut(&mut self) -> &mut Self::Target {
                &mut self.0
            }
        }

        impl std::borrow::Borrow<[u8]> for $type {
            fn borrow(&self) -> &[u8] {
                self.0.as_slice()
            }
        }

        impl From<Vec<u8>> for $type {
            fn from(value: Vec<u8>) -> Self {
                Self(value)
            }
        }

        impl<const N: usize> From<[u8; N]> for $type {
            fn from(value: [u8; N]) -> Self {
                Self(value.to_vec())
            }
        }

        impl From<&[u8]> for $type {
            fn from(value: &[u8]) -> Self {
                Self(value.to_vec())
            }
        }

        impl<const N: usize> From<&[u8; N]> for $type {
            fn from(value: &[u8; N]) -> Self {
                Self(value.to_vec())
            }
        }

        impl From<$type> for Vec<u8> {
            fn from(value: $type) -> Self {
                value.0
            }
        }

        impl AsRef<[u8]> for $type {
            fn as_ref(&self) -> &[u8] {
                &self.0
            }
        }

        impl AsMut<[u8]> for $type {
            fn as_mut(&mut self) -> &mut [u8] {
                &mut self.0
            }
        }

        macro_rules! partial_eq_impl {
            ($other:ty) => {
                impl PartialEq<$other> for $type {
                    fn eq(&self, other: &$other) -> bool {
                        self.as_slice() == &other[..]
                    }
                }

                impl PartialEq<$type> for $other {
                    fn eq(&self, other: &$type) -> bool {
                        self.eq(&other.0)
                    }
                }
            };
        }

        partial_eq_impl!(Vec<u8>);
        partial_eq_impl!([u8]);
        partial_eq_impl!(&[u8]);

        impl<const N: usize> PartialEq<[u8; N]> for $type {
            fn eq(&self, other: &[u8; N]) -> bool {
                self.0.eq(other)
            }
        }

        impl<const N: usize> PartialEq<$type> for [u8; N] {
            fn eq(&self, other: &$type) -> bool {
                self.as_slice().eq(&other.0)
            }
        }

        pastey::paste! {
            #[doc(hidden)]
            struct [<$type Visitor>];

            impl<'de> serde::de::Visitor<'de> for [<$type Visitor>] {
                type Value = $type;

                fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                    write!(
                        formatter,
                        "a url-safe base64-encoded string, bytes, or sequence of integers"
                    )
                }

                fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
                where
                    E: serde::de::Error,
                {
                    // Forgive alt base64 decoding formats
                    for config in crate::ALLOWED_DECODING_FORMATS {
                        if let Ok(data) = config.decode(v) {
                            return Ok(<$type>::from(data));
                        }
                    }

                    Err(serde::de::Error::invalid_value(serde::de::Unexpected::Str(v), &self))
                }

                fn visit_seq<A>(self, mut v: A) -> Result<Self::Value, A::Error>
                where
                    A: serde::de::SeqAccess<'de>,
                {
                    let mut data = if let Some(sz) = v.size_hint() {
                        Vec::with_capacity(sz)
                    } else {
                        Vec::new()
                    };

                    while let Some(i) = v.next_element()? {
                        data.push(i)
                    }
                    Ok(<$type>::from(data))
                }

                fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
                where
                    E: serde::de::Error,
                {
                    Ok(<$type>::from(v))
                }

                fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
                where
                    E: serde::de::Error,
                {
                    Ok(<$type>::from(v))
                }
            }

            impl<'de> serde::Deserialize<'de> for $type {
                fn deserialize<D>(deserializer: D) -> Result<Self, <D as serde::Deserializer<'de>>::Error>
                where
                    D: serde::Deserializer<'de>,
                {
                    // Was previously _str
                    deserializer.deserialize_any([<$type Visitor>])
                }
            }
        }
    };
}
