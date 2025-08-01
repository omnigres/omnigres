//! Types related to CBOR Object Signing and Encryption (COSE)

use serde::{Deserialize, Serialize};

/// A COSE signature algorithm, indicating the type of key and hash type
/// that should be used. You shouldn't need to alter or use this value.
#[allow(non_camel_case_types)]
#[derive(Copy, Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
#[repr(i32)]
pub enum COSEAlgorithm {
    /// Identifies this key as ECDSA (recommended SECP256R1) with SHA256 hashing
    #[serde(alias = "ECDSA_SHA256")]
    ES256 = -7, // recommends curve SECP256R1
    /// Identifies this key as ECDSA (recommended SECP384R1) with SHA384 hashing
    #[serde(alias = "ECDSA_SHA384")]
    ES384 = -35, // recommends curve SECP384R1
    /// Identifies this key as ECDSA (recommended SECP521R1) with SHA512 hashing
    #[serde(alias = "ECDSA_SHA512")]
    ES512 = -36, // recommends curve SECP521R1
    /// Identifies this key as RS256 aka RSASSA-PKCS1-v1_5 w/ SHA-256
    RS256 = -257,
    /// Identifies this key as RS384 aka RSASSA-PKCS1-v1_5 w/ SHA-384
    RS384 = -258,
    /// Identifies this key as RS512 aka RSASSA-PKCS1-v1_5 w/ SHA-512
    RS512 = -259,
    /// Identifies this key as PS256 aka RSASSA-PSS w/ SHA-256
    PS256 = -37,
    /// Identifies this key as PS384 aka RSASSA-PSS w/ SHA-384
    PS384 = -38,
    /// Identifies this key as PS512 aka RSASSA-PSS w/ SHA-512
    PS512 = -39,
    /// Identifies this key as EdDSA (likely curve ed25519)
    EDDSA = -8,
    /// Identifies this as an INSECURE RS1 aka RSASSA-PKCS1-v1_5 using SHA-1. This is not
    /// used by validators, but can exist in some windows hello tpm's
    INSECURE_RS1 = -65535,
    /// Identifies this key as the protocol used for [PIN/UV Auth Protocol One](https://fidoalliance.org/specs/fido-v2.1-ps-20210615/fido-client-to-authenticator-protocol-v2.1-ps-20210615.html#pinProto1)
    ///
    /// This reports as algorithm `-25`, but it is a lie. Don't include this in any algorithm lists.
    PinUvProtocol,
}

impl COSEAlgorithm {
    /// Return the set of secure recommended COSEAlgorithm's
    pub fn secure_algs() -> Vec<Self> {
        vec![
            COSEAlgorithm::ES256,
            // COSEAlgorithm::ES384,
            // COSEAlgorithm::ES512,
            COSEAlgorithm::RS256,
            // COSEAlgorithm::RS384,
            // COSEAlgorithm::RS512
            // -- Testing required
            // COSEAlgorithm::EDDSA,
        ]
    }

    /// Return the set of all possible algorithms that may exist as a COSEAlgorithm
    pub fn all_possible_algs() -> Vec<Self> {
        vec![
            COSEAlgorithm::ES256,
            COSEAlgorithm::ES384,
            COSEAlgorithm::ES512,
            COSEAlgorithm::RS256,
            COSEAlgorithm::RS384,
            COSEAlgorithm::RS512,
            COSEAlgorithm::PS256,
            COSEAlgorithm::PS384,
            COSEAlgorithm::PS512,
            COSEAlgorithm::EDDSA,
            COSEAlgorithm::INSECURE_RS1,
        ]
    }
}

impl TryFrom<i128> for COSEAlgorithm {
    type Error = ();

    fn try_from(i: i128) -> Result<Self, Self::Error> {
        match i {
            -7 => Ok(COSEAlgorithm::ES256),
            -35 => Ok(COSEAlgorithm::ES384),
            -36 => Ok(COSEAlgorithm::ES512),
            -257 => Ok(COSEAlgorithm::RS256),
            -258 => Ok(COSEAlgorithm::RS384),
            -259 => Ok(COSEAlgorithm::RS512),
            -37 => Ok(COSEAlgorithm::PS256),
            -38 => Ok(COSEAlgorithm::PS384),
            -39 => Ok(COSEAlgorithm::PS512),
            -8 => Ok(COSEAlgorithm::EDDSA),
            -65535 => Ok(COSEAlgorithm::INSECURE_RS1),
            _ => Err(()),
        }
    }
}
