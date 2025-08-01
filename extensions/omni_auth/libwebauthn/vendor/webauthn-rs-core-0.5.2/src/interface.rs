//! Extended Structs and representations for Webauthn Operations. These types are designed
//! to allow persistence and should not change.

use crate::attestation::verify_attestation_ca_chain;
use crate::error::*;
pub use crate::internals::AttestationObject;
use std::fmt;
use webauthn_rs_proto::cose::*;
use webauthn_rs_proto::extensions::*;
use webauthn_rs_proto::options::*;

pub use webauthn_attestation_ca::*;

use base64urlsafedata::HumanBinaryData;

use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

use openssl::{bn, ec, nid, pkey, x509};
use uuid::Uuid;

/// Representation of an AAGUID
/// <https://www.w3.org/TR/webauthn/#aaguid>
pub type Aaguid = [u8; 16];

/// Representation of a credentials activation counter.
pub type Counter = u32;

/// The in progress state of a credential registration attempt. You must persist this in a server
/// side location associated to the active session requesting the registration. This contains the
/// user unique id which you can use to reference the user requesting the registration.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RegistrationState {
    pub(crate) policy: UserVerificationPolicy,
    pub(crate) exclude_credentials: Vec<CredentialID>,
    pub(crate) challenge: HumanBinaryData,
    pub(crate) credential_algorithms: Vec<COSEAlgorithm>,
    pub(crate) require_resident_key: bool,
    pub(crate) authenticator_attachment: Option<AuthenticatorAttachment>,
    pub(crate) extensions: RequestRegistrationExtensions,
    pub(crate) allow_synchronised_authenticators: bool,
}

/// The in progress state of an authentication attempt. You must persist this associated to the UserID
/// requesting the registration.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AuthenticationState {
    pub(crate) credentials: Vec<Credential>,
    pub(crate) policy: UserVerificationPolicy,
    pub(crate) challenge: HumanBinaryData,
    pub(crate) appid: Option<String>,
    pub(crate) allow_backup_eligible_upgrade: bool,
}

impl AuthenticationState {
    /// set which credentials the user is allowed to authenticate with. This
    /// is used as part of resident key authentication flows where we need
    /// to inject the set of viable credentials after the client has sent us
    /// their public key credential and we identify the user.
    pub fn set_allowed_credentials(&mut self, credentials: Vec<Credential>) {
        self.credentials = credentials;
    }
}

/// An EDDSACurve identifier. You probably will never need to alter
/// or use this value, as it is set inside the Credential for you.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub enum EDDSACurve {
    // +---------+-------+----------+------------------------------------+
    // | Name    | Value | Key Type | Description                        |
    // +---------+-------+----------+------------------------------------+
    // | X25519  | 4     | OKP      | X25519 for use w/ ECDH only        |
    // | X448    | 5     | OKP      | X448 for use w/ ECDH only          |
    // | Ed25519 | 6     | OKP      | Ed25519 for use w/ EdDSA only      |
    // | Ed448   | 7     | OKP      | Ed448 for use w/ EdDSA only        |
    // +---------+-------+----------+------------------------------------+
    // /// Identifies this curve as X25519 ECDH only
    // X25519 = 4,
    // /// Identifies this curve as X448 ECDH only
    // X448 = 5,
    /// Identifies this OKP as ED25519
    ED25519 = 6,
    /// Identifies this OKP as ED448
    ED448 = 7,
}

impl EDDSACurve {
    /// Returns the size in bytes of the coordinate for the specified curve
    pub(crate) fn coordinate_size(&self) -> usize {
        match self {
            Self::ED25519 => 32,
            Self::ED448 => 57,
        }
    }
}

/// An ECDSACurve identifier. You probably will never need to alter
/// or use this value, as it is set inside the Credential for you.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub enum ECDSACurve {
    // +---------+-------+----------+------------------------------------+
    // | Name    | Value | Key Type | Description                        |
    // +---------+-------+----------+------------------------------------+
    // | P-256   | 1     | EC2      | NIST P-256 also known as secp256r1 |
    // | P-384   | 2     | EC2      | NIST P-384 also known as secp384r1 |
    // | P-521   | 3     | EC2      | NIST P-521 also known as secp521r1 |
    // +---------+-------+----------+------------------------------------+
    /// Identifies this curve as SECP256R1 (X9_62_PRIME256V1 in OpenSSL)
    SECP256R1 = 1,
    /// Identifies this curve as SECP384R1
    SECP384R1 = 2,
    /// Identifies this curve as SECP521R1
    SECP521R1 = 3,
}

impl ECDSACurve {
    /// Returns the size in bytes of the coordinate components (x and y) for the specified curve
    pub(crate) fn coordinate_size(&self) -> usize {
        match self {
            Self::SECP256R1 => 32,
            Self::SECP384R1 => 48,
            Self::SECP521R1 => 66,
        }
    }
}

impl From<&ECDSACurve> for nid::Nid {
    fn from(c: &ECDSACurve) -> Self {
        use ECDSACurve::*;
        match c {
            SECP256R1 => nid::Nid::X9_62_PRIME256V1,
            SECP384R1 => nid::Nid::SECP384R1,
            SECP521R1 => nid::Nid::SECP521R1,
        }
    }
}

/// A COSE Elliptic Curve Public Key. This is generally the provided credential
/// that an authenticator registers, and is used to authenticate the user.
/// You will likely never need to interact with this value, as it is part of the Credential
/// API.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub struct COSEEC2Key {
    /// The curve that this key references.
    pub curve: ECDSACurve,
    /// The key's public X coordinate.
    pub x: HumanBinaryData,
    /// The key's public Y coordinate.
    pub y: HumanBinaryData,
}

impl TryFrom<&COSEEC2Key> for ec::EcKey<pkey::Public> {
    type Error = openssl::error::ErrorStack;

    fn try_from(k: &COSEEC2Key) -> Result<Self, Self::Error> {
        let group = ec::EcGroup::from_curve_name((&k.curve).into())?;
        let mut ctx = bn::BigNumContext::new()?;
        let mut point = ec::EcPoint::new(&group)?;
        let x = bn::BigNum::from_slice(k.x.as_slice())?;
        let y = bn::BigNum::from_slice(k.y.as_slice())?;
        point.set_affine_coordinates_gfp(&group, &x, &y, &mut ctx)?;

        ec::EcKey::from_public_key(&group, &point)
    }
}

/// A COSE Elliptic Curve Public Key. This is generally the provided credential
/// that an authenticator registers, and is used to authenticate the user.
/// You will likely never need to interact with this value, as it is part of the Credential
/// API.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub struct COSEOKPKey {
    /// The curve that this key references.
    pub curve: EDDSACurve,
    /// The key's public X coordinate.
    pub x: HumanBinaryData,
}

/// A COSE RSA PublicKey. This is a provided credential from a registered
/// authenticator.
/// You will likely never need to interact with this value, as it is part of the Credential
/// API.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub struct COSERSAKey {
    /// An RSA modulus
    pub n: HumanBinaryData,
    /// An RSA exponent
    pub e: [u8; 3],
}

/// The type of Key contained within a COSE value. You should never need
/// to alter or change this type.
#[allow(non_camel_case_types)]
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub enum COSEKeyType {
    //    +-----------+-------+-----------------------------------------------+
    //    | Name      | Value | Description                                   |
    //    +-----------+-------+-----------------------------------------------+
    //    | OKP       | 1     | Octet Key Pair                                |
    //    | EC2       | 2     | Elliptic Curve Keys w/ x- and y-coordinate    |
    //    |           |       | pair                                          |
    //    | Symmetric | 4     | Symmetric Keys                                |
    //    | Reserved  | 0     | This value is reserved                        |
    //    +-----------+-------+-----------------------------------------------+
    /// Identifies this as an Elliptic Curve octet key pair
    EC_OKP(COSEOKPKey),
    /// Identifies this as an Elliptic Curve EC2 key
    EC_EC2(COSEEC2Key),
    // EC_Symmetric,
    // EC_Reserved, // should always be invalid.
    /// Identifies this as an RSA key
    RSA(COSERSAKey),
}

/// The numeric if of the COSEKeyType used in the CBOR fields.
#[allow(non_camel_case_types)]
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[repr(i64)]
pub enum COSEKeyTypeId {
    /// Reserved
    EC_Reserved = 0,
    /// Octet Key Pair
    EC_OKP = 1,
    /// Elliptic Curve Keys w/ x- and y-coordinate
    EC_EC2 = 2,
    /// RSA
    EC_RSA = 3,
    /// Symmetric
    EC_Symmetric = 4,
}

/// A COSE Key as provided by the Authenticator. You should never need
/// to alter or change these values.
#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub struct COSEKey {
    /// The type of key that this contains
    pub type_: COSEAlgorithm,
    /// The public key
    pub key: COSEKeyType,
}

/// The ID of this Credential
pub type CredentialID = HumanBinaryData;

/// The current latest Credential Format
pub type Credential = CredentialV5;

/// A user's authenticator credential. It contains an id, the public key
/// and a counter of how many times the authenticator has been used.
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct CredentialV5 {
    /// The ID of this credential.
    pub cred_id: CredentialID,
    /// The public key of this credential
    pub cred: COSEKey,
    /// The counter for this credential
    pub counter: Counter,
    /// The set of transports this credential indicated it could use. This is NOT
    /// a security property, but a hint for the browser and the user experience to
    /// how to communicate to this specific device.
    pub transports: Option<Vec<AuthenticatorTransport>>,
    /// During registration, if this credential was verified
    /// then this is true. If not it is false. This is based on
    /// the policy at the time of registration of the credential.
    ///
    /// This is a deviation from the Webauthn specification, because
    /// it clarifies the user experience of the credentials to UV
    /// being a per-credential attribute, rather than a per-authentication
    /// ceremony attribute. For example it can be surprising to register
    /// a credential as un-verified but then to use verification with it
    /// in the future.
    pub user_verified: bool,
    /// During registration, this credential indicated that it *may* be possible
    /// for it to exist between multiple hardware authenticators, or be backed up.
    ///
    /// This means the private key is NOT sealed within a hardware cryptograhic
    /// processor, and may have impacts on your risk assessments and modeling.
    pub backup_eligible: bool,
    /// This credential has indicated that it is currently backed up OR that it
    /// is shared between multiple devices.
    pub backup_state: bool,
    /// During registration, the policy that was requested from this
    /// credential. This is used to understand if the how the verified
    /// component interacts with the device, i.e. an always verified authenticator
    /// vs one that can dynamically request it.
    pub registration_policy: UserVerificationPolicy,
    /// The set of extensions that were verified at registration, that can
    /// be used in future authentication attempts
    pub extensions: RegisteredExtensions,
    /// The attestation certificate of this credential, including parsed metadata from the
    /// credential.
    pub attestation: ParsedAttestation,
    /// the format of the attestation
    pub attestation_format: AttestationFormat,
}

impl Credential {
    /// Re-verify this Credential's attestation chain. This re-applies the same process
    /// for certificate authority verification that occurred at registration. This can
    /// be useful if you want to re-assert your credentials match an updated or changed
    /// ca_list from the time that registration occurred. This can also be useful to
    /// re-determine certain properties of your device that may exist.
    pub fn verify_attestation<'a>(
        &'_ self,
        ca_list: &'a AttestationCaList,
    ) -> Result<Option<&'a AttestationCa>, WebauthnError> {
        // Formerly we disabled this due to apple, but they no longer provide
        // meaningful attestation so we can re-enable it.
        let danger_disable_certificate_time_checks = false;
        verify_attestation_ca_chain(
            &self.attestation.data,
            ca_list,
            danger_disable_certificate_time_checks,
        )
    }
}

impl From<CredentialV3> for Credential {
    fn from(other: CredentialV3) -> Credential {
        let CredentialV3 {
            cred_id,
            cred,
            counter,
            verified,
            registration_policy,
        } = other;

        // prior to 20220520 no multi-device credentials existed to migrate from.
        Credential {
            cred_id: HumanBinaryData::from(cred_id),
            cred,
            counter,
            transports: None,
            user_verified: verified,
            backup_eligible: false,
            backup_state: false,
            registration_policy,
            extensions: RegisteredExtensions::none(),
            attestation: ParsedAttestation {
                data: ParsedAttestationData::None,
                metadata: AttestationMetadata::None,
            },
            attestation_format: AttestationFormat::None,
        }
    }
}

/// A legacy serialisation from version 3 of Webauthn RS. Only useful for migrations.
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct CredentialV3 {
    /// The ID of this credential.
    pub cred_id: Vec<u8>,
    /// The public key of this credential
    pub cred: COSEKey,
    /// The counter for this credential
    pub counter: u32,
    /// During registration, if this credential was verified
    /// then this is true. If not it is false. This is based on
    /// the policy at the time of registration of the credential.
    ///
    /// This is a deviation from the Webauthn specification, because
    /// it clarifies the user experience of the credentials to UV
    /// being a per-credential attribute, rather than a per-authentication
    /// ceremony attribute. For example it can be surprising to register
    /// a credential as un-verified but then to use verification with it
    /// in the future.
    pub verified: bool,
    /// During registration, the policy that was requested from this
    /// credential. This is used to understand if the how the verified
    /// component interacts with the device, IE an always verified authenticator
    /// vs one that can dynamically request it.
    pub registration_policy: UserVerificationPolicy,
}

/// Serialised Attestation Data which can be stored in a stable database or similar.
#[derive(Clone, Serialize, Deserialize)]
pub enum SerialisableAttestationData {
    /// See [ParsedAttestationData::Basic]
    Basic(Vec<HumanBinaryData>),
    /// See [ParsedAttestationData::Self_]
    Self_,
    /// See [ParsedAttestationData::AttCa]
    AttCa(Vec<HumanBinaryData>),
    /// See [ParsedAttestationData::AnonCa]
    AnonCa(Vec<HumanBinaryData>),
    /// See [ParsedAttestationData::ECDAA]
    ECDAA,
    /// See [ParsedAttestationData::None]
    None,
    /// See [ParsedAttestationData::Uncertain]
    Uncertain,
}

impl fmt::Debug for SerialisableAttestationData {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            SerialisableAttestationData::Basic(_) => {
                write!(f, "SerialisableAttestationData::Basic")
            }
            SerialisableAttestationData::Self_ => write!(f, "SerialisableAttestationData::Self_"),
            SerialisableAttestationData::AttCa(_) => {
                write!(f, "SerialisableAttestationData::AttCa")
            }
            SerialisableAttestationData::AnonCa(_) => {
                write!(f, "SerialisableAttestationData::AnonCa")
            }
            SerialisableAttestationData::ECDAA => write!(f, "SerialisableAttestationData::ECDAA"),
            SerialisableAttestationData::None => write!(f, "SerialisableAttestationData::None"),
            SerialisableAttestationData::Uncertain => {
                write!(f, "SerialisableAttestationData::Uncertain")
            }
        }
    }
}

/// The processed attestation and its metadata
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ParsedAttestation {
    /// the attestation chain data
    pub data: ParsedAttestationData,
    /// possible metadata (i.e. flags set) about the attestation
    pub metadata: AttestationMetadata,
}

impl Default for ParsedAttestation {
    fn default() -> Self {
        ParsedAttestation {
            data: ParsedAttestationData::None,
            metadata: AttestationMetadata::None,
        }
    }
}

/// The processed Attestation that the Authenticator is providing in its AttestedCredentialData. This
/// metadata may allow identification of the device and its specific properties.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum AttestationMetadata {
    /// no metadata available for this device.
    None,
    /// This is commonly found on Fido Authenticators.
    Packed {
        /// This is the unique id of the class/type of device. Often this id can imply the
        /// properties of the device.
        aaguid: Uuid,
    },
    /// This is found on TPM authenticators.
    Tpm {
        /// This is the unique id of the class/type of device. Often this id can imply the
        /// properties of the device.
        aaguid: Uuid,
        /// The firmware version of the device at registration. It can NOT be determined
        /// if this updates later, which may require you to re-register the device if
        /// you need to enforce a version update.
        firmware_version: u64,
    },
    /// various attestation flags set by the device (attested by OS)
    AndroidKey {
        /// is the key master running in a Trusted Execution Environment
        is_km_tee: bool,
        /// did the attestation come from a Trusted Execution Environment
        is_attest_tee: bool,
    },
    /// various attestation flags set by the device (attested via safety-net)
    /// <https://developer.android.com/training/safetynet/attestation#use-response-server>
    AndroidSafetyNet {
        /// the name of apk that originated this key operation
        apk_package_name: String,
        /// cert chain for this apk
        apk_certificate_digest_sha256: Vec<HumanBinaryData>,
        /// A stricter verdict of device integrity. If the value of ctsProfileMatch is true, then the profile of the device running your app matches the profile of a device that has passed Android compatibility testing and has been approved as a Google-certified Android device.
        cts_profile_match: bool,
        /// A more lenient verdict of device integrity. If only the value of basicIntegrity is true, then the device running your app likely wasn't tampered with. However, the device hasn't necessarily passed Android compatibility testing.
        basic_integrity: bool,
        /// Types of measurements that contributed to the current API response
        evaluation_type: Option<String>,
    },
}

/// The processed Attestation that the Authenticator is providing in its AttestedCredentialData
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(
    try_from = "SerialisableAttestationData",
    into = "SerialisableAttestationData"
)]
pub enum ParsedAttestationData {
    /// The credential is authenticated by a signing X509 Certificate
    /// from a vendor or provider.
    Basic(Vec<x509::X509>),
    /// The credential is authenticated using surrogate basic attestation
    /// it uses the credential private key to create the attestation signature
    Self_,
    /// The credential is authenticated using a CA, and may provide a
    /// ca chain to validate to its root.
    AttCa(Vec<x509::X509>),
    /// The credential is authenticated using an anonymization CA, and may provide a ca chain to
    /// validate to its root.
    AnonCa(Vec<x509::X509>),
    /// Unimplemented
    ECDAA,
    /// No Attestation type was provided with this Credential. If in doubt
    /// reject this Credential.
    None,
    /// Uncertain Attestation was provided with this Credential, which may not
    /// be trustworthy in all cases. If in doubt, reject this type.
    Uncertain,
}

#[allow(clippy::from_over_into)]
impl Into<SerialisableAttestationData> for ParsedAttestationData {
    fn into(self) -> SerialisableAttestationData {
        match self {
            ParsedAttestationData::Basic(chain) => SerialisableAttestationData::Basic(
                chain
                    .into_iter()
                    .map(|c| HumanBinaryData::from(c.to_der().expect("Invalid DER")))
                    .collect(),
            ),
            ParsedAttestationData::Self_ => SerialisableAttestationData::Self_,
            ParsedAttestationData::AttCa(chain) => SerialisableAttestationData::AttCa(
                // HumanBinaryData::from(c.to_der().expect("Invalid DER")),
                chain
                    .into_iter()
                    .map(|c| HumanBinaryData::from(c.to_der().expect("Invalid DER")))
                    .collect(),
            ),
            ParsedAttestationData::AnonCa(chain) => SerialisableAttestationData::AnonCa(
                // HumanBinaryData::from(c.to_der().expect("Invalid DER")),
                chain
                    .into_iter()
                    .map(|c| HumanBinaryData::from(c.to_der().expect("Invalid DER")))
                    .collect(),
            ),
            ParsedAttestationData::ECDAA => SerialisableAttestationData::ECDAA,
            ParsedAttestationData::None => SerialisableAttestationData::None,
            ParsedAttestationData::Uncertain => SerialisableAttestationData::Uncertain,
        }
    }
}

impl TryFrom<SerialisableAttestationData> for ParsedAttestationData {
    type Error = WebauthnError;

    fn try_from(data: SerialisableAttestationData) -> Result<Self, Self::Error> {
        Ok(match data {
            SerialisableAttestationData::Basic(chain) => ParsedAttestationData::Basic(
                chain
                    .into_iter()
                    .map(|c| {
                        x509::X509::from_der(c.as_slice()).map_err(WebauthnError::OpenSSLError)
                    })
                    .collect::<WebauthnResult<_>>()?,
            ),
            SerialisableAttestationData::Self_ => ParsedAttestationData::Self_,
            SerialisableAttestationData::AttCa(chain) => ParsedAttestationData::AttCa(
                // x509::X509::from_der(&c.0).map_err(WebauthnError::OpenSSLError)?,
                chain
                    .into_iter()
                    .map(|c| {
                        x509::X509::from_der(c.as_slice()).map_err(WebauthnError::OpenSSLError)
                    })
                    .collect::<WebauthnResult<_>>()?,
            ),
            SerialisableAttestationData::AnonCa(chain) => ParsedAttestationData::AnonCa(
                // x509::X509::from_der(&c.0).map_err(WebauthnError::OpenSSLError)?,
                chain
                    .into_iter()
                    .map(|c| {
                        x509::X509::from_der(c.as_slice()).map_err(WebauthnError::OpenSSLError)
                    })
                    .collect::<WebauthnResult<_>>()?,
            ),
            SerialisableAttestationData::ECDAA => ParsedAttestationData::ECDAA,
            SerialisableAttestationData::None => ParsedAttestationData::None,
            SerialisableAttestationData::Uncertain => ParsedAttestationData::Uncertain,
        })
    }
}

/// Marker type parameter for data related to registration ceremony
#[derive(Debug)]
pub struct Registration;

/// Marker type parameter for data related to authentication ceremony
#[derive(Debug)]
pub struct Authentication;

/// Trait for ceremony marker structs
pub trait Ceremony {
    /// The type of the extension outputs of the ceremony
    type SignedExtensions: DeserializeOwned + std::fmt::Debug + std::default::Default;
}

impl Ceremony for Registration {
    type SignedExtensions = RegistrationSignedExtensions;
}

impl Ceremony for Authentication {
    type SignedExtensions = AuthenticationSignedExtensions;
}

/// The client's response to the request that it use the `credProtect` extension
///
/// Implemented as wrapper struct to (de)serialize
/// [CredentialProtectionPolicy] as a number
#[derive(Debug, Serialize, Clone, Deserialize)]
#[serde(try_from = "u8", into = "u8")]
pub struct CredProtectResponse(pub CredentialProtectionPolicy);

/// The output for registration ceremony extensions.
///
/// Implements the registration bits of \[AuthenticatorExtensionsClientOutputs\]
/// from the spec
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct RegistrationSignedExtensions {
    /// The `credProtect` extension
    #[serde(rename = "credProtect")]
    pub cred_protect: Option<CredProtectResponse>,
    /// The `hmac-secret` extension response to a create request
    #[serde(rename = "hmac-secret")]
    pub hmac_secret: Option<bool>,
    /// Extension key-values that we have parsed, but don't strictly recognise.
    #[serde(flatten)]
    pub unknown_keys: BTreeMap<String, serde_cbor_2::Value>,
}

/// The output for authentication cermeony extensions.
///
/// Implements the authentication bits of
/// \[AuthenticationExtensionsClientOutputs] from the spec
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct AuthenticationSignedExtensions {
    /// Extension key-values that we have parsed, but don't strictly recognise.
    #[serde(flatten)]
    pub unknown_keys: BTreeMap<String, serde_cbor_2::Value>,
}

/// Attested Credential Data
#[derive(Debug, Clone)]
pub struct AttestedCredentialData {
    /// The guid of the authenticator. May indicate manufacturer.
    pub aaguid: Aaguid,
    /// The credential ID.
    pub credential_id: CredentialID,
    /// The credentials public Key.
    pub credential_pk: serde_cbor_2::Value,
}

/// Information about the authentication that occurred.
#[derive(Debug, Serialize, Clone, Deserialize)]
pub struct AuthenticationResult {
    /// The credential ID that was used to authenticate.
    pub(crate) cred_id: CredentialID,
    /// If the credential associated needs updating
    pub(crate) needs_update: bool,
    /// If the authentication provided user_verification.
    pub(crate) user_verified: bool,
    /// The current backup state of the authenticator. It may have
    /// changed since registration.
    pub(crate) backup_state: bool,
    /// The current backup eligibility of the authenticator. It may have
    /// changed since registration in rare cases. This transition may ONLY
    /// be false to true, never the reverse. This is common on passkeys
    /// during some upgrades.
    pub(crate) backup_eligible: bool,
    /// The state of the counter
    pub(crate) counter: Counter,
    /// The response from associated extensions.
    pub(crate) extensions: AuthenticationExtensions,
}

impl AuthenticationResult {
    /// The credential ID that was used to authenticate.
    pub fn cred_id(&self) -> &CredentialID {
        &self.cred_id
    }

    /// If this authentication result should be applied to the associated
    /// credential to update its properties.
    pub fn needs_update(&self) -> bool {
        self.needs_update
    }

    /// If the authentication provided user_verification.
    pub fn user_verified(&self) -> bool {
        self.user_verified
    }

    /// The current backup state of the authenticator. It may have
    /// changed since registration.
    pub fn backup_state(&self) -> bool {
        self.backup_state
    }

    /// The current backup eligibility of the authenticator. It may have
    /// changed since registration in rare cases. This transition may ONLY
    /// be false to true, never the reverse. This is common on passkeys
    /// during some upgrades.
    pub fn backup_eligible(&self) -> bool {
        self.backup_eligible
    }

    /// The state of the counter
    pub fn counter(&self) -> Counter {
        self.counter
    }

    /// The response from associated extensions.
    pub fn extensions(&self) -> &AuthenticationExtensions {
        &self.extensions
    }
}
