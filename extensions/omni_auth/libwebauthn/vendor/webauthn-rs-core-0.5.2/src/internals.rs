//! Internal structures for parsing webauthn registrations and challenges. This *may* change
//! at anytime and should not be relied on in your library.

use crate::error::WebauthnError;
use crate::proto::*;
use serde::Deserialize;

use base64urlsafedata::{Base64UrlSafeData, HumanBinaryData};

use std::borrow::Borrow;
use std::ops::Deref;

use nom::bytes::complete::{tag, take};
use nom::combinator::cond;
use nom::combinator::{map_opt, verify};
use nom::error::ParseError;
use nom::number::complete::{be_u16, be_u32, be_u64};

/// Representation of a UserId
pub type UserId = Vec<u8>;

/// A challenge issued by the server. This contains a set of random bytes.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Challenge(Vec<u8>);

impl Challenge {
    /// Creates a new Challenge from a vector of bytes.
    pub(crate) fn new(challenge: Vec<u8>) -> Self {
        Challenge(challenge)
    }
}

impl From<Challenge> for HumanBinaryData {
    fn from(chal: Challenge) -> Self {
        HumanBinaryData::from(chal.0)
    }
}

impl From<Challenge> for Base64UrlSafeData {
    fn from(chal: Challenge) -> Self {
        Base64UrlSafeData::from(chal.0)
    }
}

impl From<HumanBinaryData> for Challenge {
    fn from(d: HumanBinaryData) -> Self {
        Challenge(d.into())
    }
}

impl<'a> From<&'a HumanBinaryData> for &'a ChallengeRef {
    fn from(d: &'a HumanBinaryData) -> Self {
        ChallengeRef::new(d.as_slice())
    }
}

impl ToOwned for ChallengeRef {
    type Owned = Challenge;

    fn to_owned(&self) -> Self::Owned {
        Challenge(self.0.to_vec())
    }
}

impl AsRef<[u8]> for ChallengeRef {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl Deref for ChallengeRef {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl Borrow<ChallengeRef> for Challenge {
    fn borrow(&self) -> &ChallengeRef {
        ChallengeRef::new(&self.0)
    }
}

impl AsRef<ChallengeRef> for Challenge {
    fn as_ref(&self) -> &ChallengeRef {
        ChallengeRef::new(&self.0)
    }
}

impl Deref for Challenge {
    type Target = ChallengeRef;

    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

/// A reference to the [Challenge] issued by the server.
/// This contains a set of random bytes.
///
/// [ChallengeRef] is the `?Sized` type that corresponds to [Challenge]
/// in the same way that [`&[u8]`] corresponds to [`Vec<u8>`].
#[derive(Debug, Eq, PartialEq, Ord, PartialOrd, Hash)]
#[repr(transparent)]
pub struct ChallengeRef([u8]);

impl ChallengeRef {
    /// Creates a new ChallengeRef from a slice
    pub fn new(challenge: &[u8]) -> &ChallengeRef {
        // SAFETY
        // Because of #[repr(transparent)], [u8] is guaranteed to have the same representation as ChallengeRef.
        // This allows safe casting between *const pointers of these types.
        unsafe { &*(challenge as *const [u8] as *const ChallengeRef) }
    }
}

impl PartialEq<Credential> for Credential {
    fn eq(&self, c: &Credential) -> bool {
        self.cred_id == c.cred_id
    }
}
#[allow(clippy::too_many_arguments)]
impl Credential {
    pub(crate) fn new(
        acd: &AttestedCredentialData,
        auth_data: &AuthenticatorData<Registration>,
        ck: COSEKey,
        registration_policy: UserVerificationPolicy,
        attestation: ParsedAttestation,
        req_extn: &RequestRegistrationExtensions,
        client_extn: &RegistrationExtensionsClientOutputs,
        attestation_format: AttestationFormat,
        transports: &Option<Vec<AuthenticatorTransport>>,
    ) -> Self {
        let cred_protect = match (
            auth_data.extensions.cred_protect.as_ref(),
            req_extn.cred_protect.is_some(),
        ) {
            (Some(credprotect), false) => ExtnState::Unsolicited(credprotect.0),
            (Some(credprotect), true) => ExtnState::Set(credprotect.0),
            (None, true) => ExtnState::Ignored,
            (None, false) => ExtnState::NotRequested,
        };

        let hmac_create_secret = match (
            auth_data.extensions.hmac_secret.as_ref(),
            req_extn.hmac_create_secret.is_some(),
        ) {
            (Some(hmac_create_secret), false) => ExtnState::Unsolicited(*hmac_create_secret),
            (Some(hmac_create_secret), true) => ExtnState::Set(*hmac_create_secret),
            (None, true) => ExtnState::Ignored,
            (None, false) => ExtnState::NotRequested,
        };

        let appid = ExtnState::NotRequested;
        /*
        let appid = match (
            client_extn.appid,
            req_extn.appid.is_some()
        ) {
            (Some(b), _) => ExtnState::Unsigned(b),
            (None, true) => ExtnState::Ignored,
            (None, false) => ExtnState::NotRequested,
        };
        */

        let cred_props = match (
            client_extn.cred_props.as_ref(),
            req_extn.cred_props.is_some(),
        ) {
            (Some(b), _) => ExtnState::Unsigned(b.clone()),
            (None, true) => ExtnState::Ignored,
            (None, false) => ExtnState::NotRequested,
        };

        let extensions = RegisteredExtensions {
            cred_protect,
            hmac_create_secret,
            appid,
            cred_props,
        };

        trace!(?extensions);
        let counter = auth_data.counter;
        let user_verified = auth_data.user_verified;
        let backup_eligible = auth_data.backup_eligible;
        let backup_state = auth_data.backup_state;

        let transports = if attestation_format == AttestationFormat::Packed
            || attestation_format == AttestationFormat::Tpm
        {
            transports.clone()
        } else {
            None
        };

        Credential {
            cred_id: acd.credential_id.clone(),
            cred: ck,
            counter,
            transports,
            user_verified,
            backup_eligible,
            backup_state,
            registration_policy,
            extensions,
            attestation,
            attestation_format,
        }
    }
}

pub(crate) fn process_authentication_extensions(
    auth_extn: &AuthenticationSignedExtensions,
) -> AuthenticationExtensions {
    trace!(?auth_extn);
    AuthenticationExtensions {}
}

/*
WebauthnError::COSEKeyInvalidAlgorithm

impl From<&COSEAlgorithm> for i64 {
    fn from(c: &COSEAlgorithm) -> Self {
        match c {
            COSEAlgorithm::ES256 => -7,
            COSEAlgorithm::ES384 => -35,
            COSEAlgorithm::ES512 => -6,
            COSEAlgorithm::RS256 => -257,
            COSEAlgorithm::RS384 => -258,
            COSEAlgorithm::RS512 => -259,
            COSEAlgorithm::PS256 => -37,
            COSEAlgorithm::PS384 => -38,
            COSEAlgorithm::PS512 => -39,
            COSEAlgorithm::EDDSA => -8,
            COSEAlgorithm::INSECURE_RS1 => -65535,
        }
    }
}
*/

impl TryFrom<i128> for ECDSACurve {
    type Error = WebauthnError;
    fn try_from(u: i128) -> Result<Self, Self::Error> {
        match u {
            1 => Ok(ECDSACurve::SECP256R1),
            2 => Ok(ECDSACurve::SECP384R1),
            3 => Ok(ECDSACurve::SECP521R1),
            _ => Err(WebauthnError::COSEKeyECDSAInvalidCurve),
        }
    }
}

impl TryFrom<i128> for EDDSACurve {
    type Error = WebauthnError;
    fn try_from(u: i128) -> Result<Self, Self::Error> {
        match u {
            6 => Ok(EDDSACurve::ED25519),
            7 => Ok(EDDSACurve::ED448),
            _ => Err(WebauthnError::COSEKeyEDDSAInvalidCurve),
        }
    }
}

fn cbor_parser(i: &[u8]) -> nom::IResult<&[u8], serde_cbor_2::Value> {
    let mut deserializer = serde_cbor_2::Deserializer::from_slice(i);
    let v = serde::de::Deserialize::deserialize(&mut deserializer).map_err(|e| {
        error!(?e, "cbor_parser");
        nom::Err::Failure(nom::error::Error::from_error_kind(
            i,
            nom::error::ErrorKind::Fail,
        ))
    })?;

    let len = deserializer.byte_offset();

    Ok((&i[len..], v))
}

fn extensions_parser<T: Ceremony>(i: &[u8]) -> nom::IResult<&[u8], T::SignedExtensions> {
    let (i, v) = cbor_parser(i)?;
    trace!(?v, "OK!");

    let v: T::SignedExtensions = serde_cbor_2::value::from_value(v).map_err(|e| {
        error!(?e, "extensions_parser");
        nom::Err::Failure(nom::error::Error::from_error_kind(
            i,
            nom::error::ErrorKind::Fail,
        ))
    })?;
    Ok((i, v))
}

fn aaguid_parser(i: &[u8]) -> nom::IResult<&[u8], Aaguid> {
    let (i, aaguid) = take(16usize)(i)?;
    Ok((i, aaguid.try_into().expect("took 16 bytes exactly")))
}

fn acd_parser(i: &[u8]) -> nom::IResult<&[u8], AttestedCredentialData> {
    let (i, aaguid) = aaguid_parser(i)?;
    let (i, cred_id_len) = be_u16(i)?;

    if usize::from(cred_id_len) > i.len() {
        warn!(
            "cred_id_len ({:?}) is larger than remaining number of bytes to parse ({:?}).",
            cred_id_len,
            i.len()
        );
    }

    let (i, cred_id) = take(cred_id_len as usize)(i)?;
    let (i, cred_pk) = cbor_parser(i)?;

    Ok((
        i,
        AttestedCredentialData {
            aaguid,
            credential_id: HumanBinaryData::from(cred_id.to_vec()),
            credential_pk: cred_pk,
        },
    ))
}
#[allow(clippy::type_complexity)]
fn authenticator_data_flags(i: &[u8]) -> nom::IResult<&[u8], (bool, bool, bool, bool, bool, bool)> {
    // Using nom for bit fields is shit, do it by hand.
    let (i, ctrl) = nom::number::complete::u8(i)?;
    let exten_pres = (ctrl & 0b1000_0000) != 0;
    let acd_pres = (ctrl & 0b0100_0000) != 0;
    let res_1 = (ctrl & 0b0010_0000) != 0;
    let bak_st = (ctrl & 0b0001_0000) != 0;
    let bak_el = (ctrl & 0b0000_1000) != 0;
    let u_ver = (ctrl & 0b0000_0100) != 0;
    let res_4 = (ctrl & 0b0000_0010) != 0;
    let u_pres = (ctrl & 0b0000_0001) != 0;

    if res_1 || res_4 {
        warn!(
            "Usage of unknown authenticator data flags detected! {:b}",
            ctrl
        );
    }

    Ok((i, (exten_pres, acd_pres, u_ver, u_pres, bak_el, bak_st)))
}

fn authenticator_data_parser<T: Ceremony>(i: &[u8]) -> nom::IResult<&[u8], AuthenticatorData<T>> {
    let (i, rp_id_hash) = take(32usize)(i)?;
    let (i, data_flags) = authenticator_data_flags(i)?;
    let (i, counter) = be_u32(i)?;
    let (i, acd) = cond(data_flags.1, acd_parser)(i)?;
    let (i, extensions) = cond(data_flags.0, extensions_parser::<T>)(i)?;
    trace!(?extensions);
    let extensions = extensions.unwrap_or_default();

    Ok((
        i,
        AuthenticatorData {
            rp_id_hash: rp_id_hash.try_into().expect("took 32 bytes from input"),
            counter,
            user_verified: data_flags.2,
            user_present: data_flags.3,
            backup_eligible: data_flags.4,
            backup_state: data_flags.5,
            acd,
            extensions,
        },
    ))
}

/// Data returned by this authenticator during registration.
#[derive(Debug, Clone)]
pub struct AuthenticatorData<T: Ceremony> {
    /// Hash of the relying party id.
    pub(crate) rp_id_hash: [u8; 32],
    /// The counter of this credentials activations.
    pub counter: u32,
    /// Flag if the user was present.
    pub user_present: bool,
    /// Flag is the user verified to the device. Implies presence.
    pub user_verified: bool,
    /// Flag defining if the authenticator *could* be backed up OR transferred
    /// between multiple devices.
    pub backup_eligible: bool,
    /// Flag defining if the authenticator *knows* it is currently backed up or
    /// present on multiple devices.
    pub backup_state: bool,
    /// The optional attestation.
    pub acd: Option<AttestedCredentialData>,
    /// Extensions supplied by the device.
    pub extensions: T::SignedExtensions,
}

impl<T: Ceremony> TryFrom<&[u8]> for AuthenticatorData<T> {
    type Error = WebauthnError;
    fn try_from(auth_data_bytes: &[u8]) -> Result<Self, Self::Error> {
        authenticator_data_parser(auth_data_bytes)
            .map_err(|e| {
                error!(?e, "try_from authenticator_data_parser");
                WebauthnError::ParseNOMFailure
            })
            .map(|(_, ad)| ad)
    }
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct AttestationObjectInner<'a> {
    pub(crate) fmt: &'a str,
    pub(crate) att_stmt: serde_cbor_2::Value,
    pub(crate) auth_data: &'a [u8],
    pub(crate) ep_att: Option<bool>,
    pub(crate) large_blob_key: Option<&'a [u8]>,
}

/// Attestation Object
#[derive(Debug)]
pub struct AttestationObject<T: Ceremony> {
    /// format.
    pub(crate) fmt: String,
    /// <https://w3c.github.io/webauthn/#generating-an-attestation-object>
    pub(crate) att_stmt: serde_cbor_2::Value,
    /// auth_data.
    pub(crate) auth_data: AuthenticatorData<T>,
    /// auth_data_bytes.
    pub(crate) auth_data_bytes: Vec<u8>,
    /// ep_att
    pub(crate) _ep_att: Option<bool>,
    /// large_blob_key
    pub(crate) _large_blob_key: Option<Vec<u8>>,
}

impl<T: Ceremony> TryFrom<&[u8]> for AttestationObject<T> {
    type Error = WebauthnError;

    fn try_from(data: &[u8]) -> Result<AttestationObject<T>, WebauthnError> {
        let v: serde_cbor_2::Value =
            serde_cbor_2::from_slice(data).map_err(WebauthnError::ParseCBORFailure)?;
        trace!(?v, "AttestationObjectInner");

        let aoi: AttestationObjectInner =
            serde_cbor_2::from_slice(data).map_err(WebauthnError::ParseCBORFailure)?;
        let auth_data_bytes: &[u8] = aoi.auth_data;
        let auth_data = AuthenticatorData::try_from(auth_data_bytes)?;

        // Yay! Now we can assemble a reasonably sane structure.
        Ok(AttestationObject {
            fmt: aoi.fmt.to_owned(),
            att_stmt: aoi.att_stmt,
            auth_data,
            auth_data_bytes: auth_data_bytes.to_owned(),
            _ep_att: aoi.ep_att,
            _large_blob_key: aoi.large_blob_key.map(|v| v.to_owned()),
        })
    }
}

impl TryFrom<u8> for CredProtectResponse {
    type Error = <CredentialProtectionPolicy as TryFrom<u8>>::Error;

    fn try_from(v: u8) -> Result<Self, Self::Error> {
        CredentialProtectionPolicy::try_from(v).map(CredProtectResponse)
    }
}

impl From<CredProtectResponse> for u8 {
    fn from(policy: CredProtectResponse) -> Self {
        policy.0 as u8
    }
}

pub(crate) struct AuthenticatorAttestationResponse<T: Ceremony> {
    pub(crate) attestation_object: AttestationObject<T>,
    pub(crate) client_data_json: CollectedClientData,
    pub(crate) client_data_json_bytes: Vec<u8>,
    pub(crate) transports: Option<Vec<AuthenticatorTransport>>,
}

impl<T: Ceremony> TryFrom<&AuthenticatorAttestationResponseRaw>
    for AuthenticatorAttestationResponse<T>
{
    type Error = WebauthnError;
    fn try_from(aarr: &AuthenticatorAttestationResponseRaw) -> Result<Self, Self::Error> {
        let ccdj = serde_json::from_slice(aarr.client_data_json.as_ref())
            .map_err(WebauthnError::ParseJSONFailure)?;

        let ao = AttestationObject::try_from(aarr.attestation_object.as_ref())?;

        Ok(AuthenticatorAttestationResponse {
            attestation_object: ao,
            client_data_json: ccdj,
            client_data_json_bytes: aarr.client_data_json.clone().into(),
            transports: aarr.transports.clone(),
        })
    }
}

#[derive(Debug)]
pub(crate) struct AuthenticatorAssertionResponse<T: Ceremony> {
    pub(crate) authenticator_data: AuthenticatorData<T>,
    pub(crate) authenticator_data_bytes: Vec<u8>,
    pub(crate) client_data: CollectedClientData,
    pub(crate) client_data_bytes: Vec<u8>,
    pub(crate) signature: Vec<u8>,
    pub(crate) _user_handle: Option<Vec<u8>>,
}

impl<T: Ceremony> TryFrom<&AuthenticatorAssertionResponseRaw>
    for AuthenticatorAssertionResponse<T>
{
    type Error = WebauthnError;
    fn try_from(aarr: &AuthenticatorAssertionResponseRaw) -> Result<Self, Self::Error> {
        Ok(AuthenticatorAssertionResponse {
            authenticator_data: AuthenticatorData::try_from(aarr.authenticator_data.as_ref())?,
            authenticator_data_bytes: aarr.authenticator_data.clone().into(),
            client_data: serde_json::from_slice(aarr.client_data_json.as_ref())
                .map_err(WebauthnError::ParseJSONFailure)?,
            client_data_bytes: aarr.client_data_json.clone().into(),
            signature: aarr.signature.clone().into(),
            _user_handle: aarr.user_handle.clone().map(|uh| uh.into()),
        })
    }
}

// ===== tpm shit show begins =====

/// A magic constant that defines that a Tpm attestation comes from a TPM
pub const TPM_GENERATED_VALUE: u32 = 0xff544347;

#[derive(Debug, PartialEq, Eq)]
#[repr(u16)]
/// Tpm statement types.
pub enum TpmSt {
    /// Unused
    RspCommand = 0x00c4,
    /// Unused
    Null = 0x8000,
    /// Unused
    NoSessions = 0x8001,
    /// Unused
    Sessions = 0x8002,
    /// Unused
    ReservedA = 0x8003,
    /// Unused
    ReservedB = 0x8004,
    /// Unused
    AttestNV = 0x8014,
    /// Unused
    AttestCommandAudit = 0x8015,
    /// Unused
    AttestSessionAudit = 0x8016,
    /// Denote that this attestation contains a certify statement.
    AttestCertify = 0x8017,
    /// Unused
    AttestQuote = 0x8018,
    /// Unused
    AttestTime = 0x8019,
    /// Unused
    AttestCreation = 0x801a,
    /// Unused
    ReservedC = 0x801b,
    /// Unused
    Creation = 0x8021,
    /// Unused
    Verified = 0x8022,
    /// Unused
    AuthSecret = 0x8023,
    /// Unused
    Hashcheck = 0x8024,
    /// Unused
    AuthSigned = 0x8025,
    /// Unused
    FUManifest = 0x8029,
}

impl TpmSt {
    fn new(v: u16) -> Option<Self> {
        match v {
            0x00c4 => Some(TpmSt::RspCommand),
            0x8000 => Some(TpmSt::Null),
            0x8001 => Some(TpmSt::NoSessions),
            0x8002 => Some(TpmSt::Sessions),
            0x8003 => Some(TpmSt::ReservedA),
            0x8004 => Some(TpmSt::ReservedB),
            0x8014 => Some(TpmSt::AttestNV),
            0x8015 => Some(TpmSt::AttestCommandAudit),
            0x8016 => Some(TpmSt::AttestSessionAudit),
            0x8017 => Some(TpmSt::AttestCertify),
            0x8018 => Some(TpmSt::AttestQuote),
            0x8019 => Some(TpmSt::AttestTime),
            0x801a => Some(TpmSt::AttestCreation),
            0x801b => Some(TpmSt::ReservedC),
            0x8021 => Some(TpmSt::Creation),
            0x8022 => Some(TpmSt::Verified),
            0x8023 => Some(TpmSt::AuthSecret),
            0x8024 => Some(TpmSt::Hashcheck),
            0x8025 => Some(TpmSt::AuthSigned),
            0x8029 => Some(TpmSt::FUManifest),
            _ => None,
        }
    }
}

#[derive(Debug)]
/// Information about the TPM's clock. May be obfuscated.
pub struct TpmsClockInfo {
    _clock: u64,
    _reset_count: u32,
    _restart_count: u32,
    _safe: bool, // u8
}

fn tpmsclockinfo_parser(i: &[u8]) -> nom::IResult<&[u8], TpmsClockInfo> {
    let (i, clock) = be_u64(i)?;
    let (i, reset_count) = be_u32(i)?;
    let (i, restart_count) = be_u32(i)?;
    let (i, safe) = nom::combinator::map(nom::number::complete::u8, |v| v != 0)(i)?;

    Ok((
        i,
        TpmsClockInfo {
            _clock: clock,
            _reset_count: reset_count,
            _restart_count: restart_count,
            _safe: safe,
        },
    ))
}

#[derive(Debug)]
/// Tpm name enumeration.
pub enum Tpm2bName {
    /// No name present
    None,
    /// A handle reference
    Handle(u32),
    /// A digest of a name
    Digest(Vec<u8>),
}

#[derive(Debug)]
/// Tpm attestation union, switched by TpmSt.
pub enum TpmuAttest {
    /// The TpmuAttest contains a certify structure.
    AttestCertify(Tpm2bName, Tpm2bName),
    // AttestNV
    // AttestCommandAudit
    // AttestSessionAudit
    // AttestQuote
    // AttestTime
    // AttestCreation
    #[allow(dead_code)]
    /// An invalid union
    Invalid,
}

#[derive(Debug)]
/// Tpm attestation structure.
pub struct TpmsAttest {
    /// Magic. Should be set to TPM_GENERATED_VALUE
    pub magic: u32,
    /// The type of attestation for typeattested.
    pub type_: TpmSt,
    /// Ignored in webauthn.
    pub qualified_signer: Tpm2bName,
    /// Ignored in webauthn.
    pub extra_data: Option<Vec<u8>>,
    /// Tpm Clock Information
    pub clock_info: TpmsClockInfo,
    /// The TPM firmware version. May be obfuscated.
    pub firmware_version: u64,
    /// The attestation.
    pub typeattested: TpmuAttest,
}

fn tpm2b_name(i: &[u8]) -> nom::IResult<&[u8], Tpm2bName> {
    let (i, case) = be_u16(i)?;
    let (i, r) = match case {
        0 => (i, Tpm2bName::None),
        4 => {
            let (i, h) = be_u32(i)?;
            (i, Tpm2bName::Handle(h))
        }
        size => {
            let (i, d) = take(size as usize)(i)?;
            (i, Tpm2bName::Digest(d.to_vec()))
        }
    };

    Ok((i, r))
}

fn tpm2b_data(i: &[u8]) -> nom::IResult<&[u8], Option<Vec<u8>>> {
    let (i, size) = be_u16(i)?;
    match size {
        0 => Ok((i, None)),
        size => {
            let (i, d) = take(size as usize)(i)?;
            Ok((i, Some(d.to_vec())))
        }
    }
}

fn tpmuattestcertify(i: &[u8]) -> nom::IResult<&[u8], TpmuAttest> {
    let (i, name) = tpm2b_name(i)?;
    let (i, qualified_name) = tpm2b_name(i)?;

    Ok((i, TpmuAttest::AttestCertify(name, qualified_name)))
}

fn tpmsattest_parser(i: &[u8]) -> nom::IResult<&[u8], TpmsAttest> {
    let (i, magic) = verify(be_u32, |x| *x == TPM_GENERATED_VALUE)(i)?;
    let (i, type_) = map_opt(be_u16, TpmSt::new)(i)?;
    let (i, qualified_signer) = tpm2b_name(i)?;
    let (i, extra_data) = tpm2b_data(i)?;
    let (i, clock_info) = tpmsclockinfo_parser(i)?;
    let (i, firmware_version) = be_u64(i)?;
    let (i, typeattested) = tpmuattestcertify(i)?;

    Ok((
        i,
        TpmsAttest {
            magic,
            type_,
            qualified_signer,
            extra_data,
            clock_info,
            firmware_version,
            typeattested,
        },
    ))
}

impl TryFrom<&[u8]> for TpmsAttest {
    type Error = WebauthnError;

    fn try_from(data: &[u8]) -> Result<TpmsAttest, WebauthnError> {
        tpmsattest_parser(data)
            .map_err(|e| {
                error!(?e, "try_from tpmsattest_parser");
                WebauthnError::ParseNOMFailure
            })
            .map(|(_, v)| v)
    }
}

#[derive(Debug, Clone, Copy)]
#[repr(u16)]
/// The tpm cryptographic algorithm that may be in use.
pub enum TpmAlgId {
    /// Error occured
    Error = 0x0000,
    /// RSA
    Rsa = 0x0001,
    /// Insecure Sha1
    Sha1 = 0x0004,
    /// Hmac
    Hmac = 0x0005,
    /// Aes
    Aes = 0x0006,
    // Mgf1 = 0x0007,
    // KeyedHash = 0x0008,
    // Xor = 0x000A,
    /// Sha256
    Sha256 = 0x000B,
    /// Sha384
    Sha384 = 0x000C,
    /// Sha512
    Sha512 = 0x000D,
    /// Null (no algorithm)
    Null = 0x0010,
    // Sm3_256 = 0x0012,
    // Sm4 = 0x0013,
    /// Rsa SSA
    RsaSSA = 0x0014,
    // RsAes = 0x0015,
    /// Rsa PSS
    RsaPSS = 0x0016,
    // Oaep = 0x0017,
    /// Ecdsa
    Ecdsa = 0x0018,
    // Ecdh = 0x0019,
    /// Ecdaa
    Ecdaa = 0x001A,
    // Sm2 = 0x001B,
    // EcSchnorr = 0x001C,
    // EcMqv = 0x001D,
    // Kdf1Sp80056A = 0x0020,
    // Kdf2 = 0x0021,
    // Kdf1Sp800108 = 0x0022,
    /// Ecc
    Ecc = 0x0023,
    // Symcipher = 0x0025,
    // Camellia = 0x0026,
    // Ctr = 0x0040,
    // Ofb = 0x0041,
    // Cbc = 0x0042,
    // Cfb = 0x0043,
    // Ecb = 0x0044,
}

impl TpmAlgId {
    fn new(v: u16) -> Option<Self> {
        trace!("TpmAlgId::new ( {:x?} )", v);
        match v {
            0x0000 => Some(TpmAlgId::Error),
            0x0001 => Some(TpmAlgId::Rsa),
            0x0004 => Some(TpmAlgId::Sha1),
            0x0005 => Some(TpmAlgId::Hmac),
            0x0006 => Some(TpmAlgId::Aes),
            0x000B => Some(TpmAlgId::Sha256),
            0x000C => Some(TpmAlgId::Sha384),
            0x000D => Some(TpmAlgId::Sha512),
            0x0010 => Some(TpmAlgId::Null),
            0x0014 => Some(TpmAlgId::RsaSSA),
            0x0016 => Some(TpmAlgId::RsaPSS),
            0x0018 => Some(TpmAlgId::Ecdsa),
            0x001A => Some(TpmAlgId::Ecdaa),
            0x0023 => Some(TpmAlgId::Ecc),
            _ => None,
        }
    }
}

// Later, this probably would be rewritten interms of the chosen
// symetric algo, but for now it's always Null
#[derive(Debug)]
/// Symmetric crypto definition. Unused in webauthn
pub struct TpmtSymDefObject {
    _algorithm: TpmAlgId,
    // keybits: Option<()>,
    // mode: Option<()>,
    // details
}

fn parse_tpmtsymdefobject(input: &[u8]) -> nom::IResult<&[u8], Option<TpmtSymDefObject>> {
    let (data, algorithm) = map_opt(be_u16, TpmAlgId::new)(input)?;
    match algorithm {
        TpmAlgId::Null => Ok((data, None)),
        _ => Err(nom::Err::Failure(nom::error::Error::from_error_kind(
            input,
            nom::error::ErrorKind::Fail,
        ))),
    }
}

#[derive(Debug)]
/// Symmetric crypto definition. Unused in webauthn
pub struct TpmtEccScheme {
    _algorithm: TpmAlgId,
}

fn parse_tpmteccscheme(input: &[u8]) -> nom::IResult<&[u8], Option<TpmtEccScheme>> {
    let (data, algorithm) = map_opt(be_u16, TpmAlgId::new)(input)?;
    match algorithm {
        TpmAlgId::Null => Ok((data, None)),
        _ => Err(nom::Err::Failure(nom::error::Error::from_error_kind(
            input,
            nom::error::ErrorKind::Fail,
        ))),
    }
}

// 11.2.5.5 TPMI_ECC_CURVE
// 6.4 TPM_ECC_CURVE
#[derive(Debug, Clone, Copy)]
#[repr(u16)]
pub(crate) enum TpmiEccCurve {
    None = 0x0000,
    NistP192 = 0x0001,
    NistP224 = 0x0002,
    NistP256 = 0x0003,
    NistP384 = 0x0004,
    NistP521 = 0x0005,
    BnP256 = 0x0010,
    BnP638 = 0x0011,
    Sm2P256 = 0x0020,
}

impl TpmiEccCurve {
    fn new(v: u16) -> Option<Self> {
        trace!("TpmiEccCurve::new ( {:x?} )", v);
        match v {
            0x0000 => Some(TpmiEccCurve::None),
            0x0001 => Some(TpmiEccCurve::NistP192),
            0x0002 => Some(TpmiEccCurve::NistP224),
            0x0003 => Some(TpmiEccCurve::NistP256),
            0x0004 => Some(TpmiEccCurve::NistP384),
            0x0005 => Some(TpmiEccCurve::NistP521),
            0x0010 => Some(TpmiEccCurve::BnP256),
            0x0011 => Some(TpmiEccCurve::BnP638),
            0x0020 => Some(TpmiEccCurve::Sm2P256),
            _ => None,
        }
    }
}

#[derive(Debug)]
/// Symmetric crypto definition. Unused in webauthn
pub struct TpmtKdfScheme {
    _algorithm: TpmAlgId,
}

fn parse_tpmtkdfscheme(input: &[u8]) -> nom::IResult<&[u8], Option<TpmtKdfScheme>> {
    let (data, algorithm) = map_opt(be_u16, TpmAlgId::new)(input)?;
    match algorithm {
        TpmAlgId::Null => Ok((data, None)),
        _ => Err(nom::Err::Failure(nom::error::Error::from_error_kind(
            input,
            nom::error::ErrorKind::Fail,
        ))),
    }
}

#[derive(Debug)]
/// The Rsa Scheme. Unused in webauthn.
pub struct TpmtRsaScheme {
    _algorithm: TpmAlgId,
    // details
}

fn parse_tpmtrsascheme(input: &[u8]) -> nom::IResult<&[u8], Option<TpmtRsaScheme>> {
    let (data, algorithm) = map_opt(be_u16, TpmAlgId::new)(input)?;
    match algorithm {
        TpmAlgId::Null => Ok((data, None)),
        _ => Err(nom::Err::Failure(nom::error::Error::from_error_kind(
            input,
            nom::error::ErrorKind::Fail,
        ))),
    }
}

#[derive(Debug)]
/// Rsa Parameters.
pub(crate) struct TpmsRsaParms {
    // TPMT_SYM_DEF_OBJECT + ALG_NULL
    _symmetric: Option<TpmtSymDefObject>,
    // TPMT_RSA_SCHEME+ (rsapss, rsassa, null)
    _scheme: Option<TpmtRsaScheme>,
    // TPMI_RSA_KEY_BITS
    _keybits: u16,
    // u32
    /// The Rsa Exponent
    _exponent: u32,
}

fn tpmsrsaparms_parser(i: &[u8]) -> nom::IResult<&[u8], TpmsRsaParms> {
    let (i, symmetric) = parse_tpmtsymdefobject(i)?;
    let (i, scheme) = parse_tpmtrsascheme(i)?;
    let (i, keybits) = be_u16(i)?;
    let (i, exponent) = be_u32(i)?;
    Ok((
        i,
        TpmsRsaParms {
            _symmetric: symmetric,
            _scheme: scheme,
            _keybits: keybits,
            _exponent: exponent,
        },
    ))
    /*
    do_parse!(
        symmetric: parse_tpmtsymdefobject >>
        scheme: parse_tpmtrsascheme >>
        keybits: u16!(nom::Endianness::Big) >>
        exponent: u32!(nom::Endianness::Big) >>
        (
        TpmsRsaParms {
            _symmetric: symmetric,
            _scheme: scheme,
            _keybits: keybits,
            exponent
        }
        )
    )
    */
}

// 12.2.3.6 TPMS_ECC_PARMS in
fn tpmseccparms_parser(i: &[u8]) -> nom::IResult<&[u8], TpmsEccParms> {
    trace!(?i);
    let (i, symmetric) = parse_tpmtsymdefobject(i)?;
    let (i, scheme) = parse_tpmteccscheme(i)?;
    let (i, curve_id) = map_opt(be_u16, TpmiEccCurve::new)(i)?;
    let (i, kdf) = parse_tpmtkdfscheme(i)?;

    Ok((
        i,
        TpmsEccParms {
            _symmetric: symmetric,
            _scheme: scheme,
            curve_id,
            _kdf: kdf,
        },
    ))
}

#[derive(Debug)]
pub(crate) struct TpmsEccParms {
    _symmetric: Option<TpmtSymDefObject>,
    _scheme: Option<TpmtEccScheme>,
    /// The ID of the ECC curve in use
    pub curve_id: TpmiEccCurve,
    _kdf: Option<TpmtKdfScheme>,
}

#[derive(Debug)]
/// Asymmetric Public Parameters
pub(crate) enum TpmuPublicParms {
    // KeyedHash
    // Symcipher
    /// Rsa
    Rsa(TpmsRsaParms),
    Ecc(TpmsEccParms),
    // Asym
}

fn parse_tpmupublicparms(input: &[u8], alg: TpmAlgId) -> nom::IResult<&[u8], TpmuPublicParms> {
    trace!(?input, ?alg, "tpmupublicparms input");
    match alg {
        TpmAlgId::Rsa => {
            tpmsrsaparms_parser(input).map(|(data, inner)| (data, TpmuPublicParms::Rsa(inner)))
        }
        TpmAlgId::Ecc => {
            tpmseccparms_parser(input).map(|(data, inner)| (data, TpmuPublicParms::Ecc(inner)))
        }
        a => {
            debug!(?a, "unsuported alg in parse_tpmupublicparms");
            Err(nom::Err::Failure(nom::error::Error::from_error_kind(
                input,
                nom::error::ErrorKind::Fail,
            )))
        }
    }
}

/// 11.2.5.2 TPMS_ECC_POINT
#[derive(Debug)]
pub(crate) struct TpmsEccPoint {
    pub x: Vec<u8>,
    pub y: Vec<u8>,
}

fn tpmseccpoint_parser(i: &[u8]) -> nom::IResult<&[u8], TpmsEccPoint> {
    let (i, size) = be_u16(i)?;
    let (i, x) = match size {
        0 => (i, Vec::new()),
        size => {
            let (i, d) = take(size as usize)(i)?;
            (i, d.to_vec())
        }
    };
    let (i, size) = be_u16(i)?;
    let (i, y) = match size {
        0 => (i, Vec::new()),
        size => {
            let (i, d) = take(size as usize)(i)?;
            (i, d.to_vec())
        }
    };
    Ok((i, TpmsEccPoint { x, y }))
}

#[derive(Debug)]
/// Asymmetric Public Key
pub(crate) enum TpmuPublicId {
    // KeyedHash
    // Symcipher
    /// Rsa
    Rsa(Vec<u8>),
    /// Ecc
    Ecc(TpmsEccPoint),
    // Asym
}

fn tpmsrsapublickey_parser(i: &[u8]) -> nom::IResult<&[u8], Vec<u8>> {
    let (i, size) = be_u16(i)?;
    match size {
        0 => Ok((i, Vec::new())),
        size => {
            let (i, d) = take(size as usize)(i)?;
            Ok((i, d.to_vec()))
        }
    }
}

fn parse_tpmupublicid(input: &[u8], alg: TpmAlgId) -> nom::IResult<&[u8], TpmuPublicId> {
    // eprintln!("tpmupublicparms input -> {:?}", input);
    match alg {
        TpmAlgId::Rsa => {
            tpmsrsapublickey_parser(input).map(|(data, inner)| (data, TpmuPublicId::Rsa(inner)))
        }
        TpmAlgId::Ecc => {
            tpmseccpoint_parser(input).map(|(data, inner)| (data, TpmuPublicId::Ecc(inner)))
        }
        a => {
            debug!(?a, "unsuported alg in parse_tpmupublicid");
            Err(nom::Err::Failure(nom::error::Error::from_error_kind(
                input,
                nom::error::ErrorKind::Fail,
            )))
        }
    }
}

#[derive(Debug)]
/// Tpm Public Key Structure
pub(crate) struct TpmtPublic {
    /// The type of public parms and key IE Ecdsa or Rsa
    pub _type: TpmAlgId,
    /// The hash type over pubarea (webauthn specific)
    pub name_alg: TpmAlgId,
    // TPMA_OBJECT
    /// Unused in webauthn.
    pub _object_attributes: u32,
    /// Unused in webauthn.
    pub _auth_policy: Option<Vec<u8>>,
    //
    // TPMU_PUBLIC_PARMS
    /// Public Parameters
    pub parameters: TpmuPublicParms,
    // TPMU_PUBLIC_ID
    /// Public Key
    pub unique: TpmuPublicId,
}

impl TryFrom<&[u8]> for TpmtPublic {
    type Error = WebauthnError;

    fn try_from(data: &[u8]) -> Result<TpmtPublic, WebauthnError> {
        trace!(?data);
        tpmtpublic_parser(data)
            .map_err(|e| {
                error!(?e, "try_from tpmtpublic_parser");
                WebauthnError::ParseNOMFailure
            })
            .map(|(_, v)| v)
    }
}

fn tpm2b_digest(i: &[u8]) -> nom::IResult<&[u8], Option<Vec<u8>>> {
    let (i, size) = be_u16(i)?;
    match size {
        0 => Ok((i, None)),
        size => {
            let (i, d) = take(size as usize)(i)?;
            Ok((i, Some(d.to_vec())))
        }
    }
}

fn tpmtpublic_parser(i: &[u8]) -> nom::IResult<&[u8], TpmtPublic> {
    let (i, type_) = map_opt(be_u16, TpmAlgId::new)(i)?;
    let (i, name_alg) = map_opt(be_u16, TpmAlgId::new)(i)?;
    let (i, _object_attributes) = be_u32(i)?;
    let (i, _auth_policy) = tpm2b_digest(i)?;
    let (i, parameters) = parse_tpmupublicparms(i, type_)?;
    let (i, unique) = parse_tpmupublicid(i, type_)?;

    Ok((
        i,
        TpmtPublic {
            _type: type_,
            name_alg,
            _object_attributes,
            _auth_policy,
            parameters,
            unique,
        },
    ))
}

#[derive(Debug)]
/// A TPM Signature.
pub enum TpmtSignature {
    // if sigAlg has a type, parse as:
    // signature - TPMU_SIGNATURE
    // Else, due to how this work if no alg, just pass the raw bytes back.
    /// A raw signature, verifyied by a cert + hash combination. May be implementation
    /// specific.
    RawSignature(Vec<u8>),
}

impl TryFrom<&[u8]> for TpmtSignature {
    type Error = WebauthnError;

    fn try_from(data: &[u8]) -> Result<TpmtSignature, WebauthnError> {
        tpmtsignature_parser(data)
            .map_err(|e| {
                error!(?e, "try_from tpmtsignature_parser");
                WebauthnError::ParseNOMFailure
            })
            .map(|(_, v)| v)
    }
}

fn tpmtsignature_parser(input: &[u8]) -> nom::IResult<&[u8], TpmtSignature> {
    let (_data, algorithm) = nom::combinator::map(be_u16, TpmAlgId::new)(input)?;
    match algorithm {
        None => Ok((&[], TpmtSignature::RawSignature(Vec::from(input)))),
        _ => Err(nom::Err::Failure(nom::error::Error::from_error_kind(
            input,
            nom::error::ErrorKind::Fail,
        ))),
    }
}

/// TPM vendor identifier, from [the TPM Vendor ID Registry][1]
///
/// [1]: https://trustedcomputinggroup.org/wp-content/uploads/TCG-TPM-VendorIDRegistry-v1p06-r0p91-pub.pdf
#[derive(Debug)]
#[allow(clippy::upper_case_acronyms)]
pub(crate) enum TpmVendor {
    AMD,
    Atmel,
    Broadcom,
    Cisco,
    FlySliceTechnologies,
    FuzhouRockchip,
    Google,
    HPE,
    Huawei,
    IBM,
    Infineon,
    Intel,
    Lenovo,
    Microsoft,
    NationalSemi,
    Nationz,
    NuvotonTechnology,
    Qualcomm,
    Samsung,
    Sinosun,
    SMSC,
    StMicroelectronics,
    TexasInstruments,
    Winbond,
}

// The value of the TPMManufacturer attribute MUST be the ASCII representation
// of the hexadecimal value of the 4 byte vendor identifier defined in the TCG
// Vendor ID Registry. Each byte is represented individually as a two digit
// unsigned hexadecimal number using the characters 0-9 and A-F.
impl TryFrom<&[u8; 8]> for TpmVendor {
    type Error = WebauthnError;

    fn try_from(v: &[u8; 8]) -> Result<Self, Self::Error> {
        match v {
            b"414d4400" => Ok(Self::AMD),
            b"41544D4C" => Ok(Self::Atmel),
            b"4252434D" => Ok(Self::Broadcom),
            b"4353434F" => Ok(Self::Cisco),
            b"464C5953" => Ok(Self::FlySliceTechnologies),
            b"524F4343" => Ok(Self::FuzhouRockchip),
            b"474F4F47" => Ok(Self::Google),
            b"48504500" => Ok(Self::HPE),
            b"48495349" => Ok(Self::Huawei),
            b"49424D00" => Ok(Self::IBM),
            b"49465800" => Ok(Self::Infineon),
            b"494E5443" => Ok(Self::Intel),
            b"4C454E00" => Ok(Self::Lenovo),
            b"4D534654" => Ok(Self::Microsoft),
            b"4E534D20" => Ok(Self::NationalSemi),
            b"4E545A00" => Ok(Self::Nationz),
            b"4E544300" => Ok(Self::NuvotonTechnology),
            b"51434F4D" => Ok(Self::Qualcomm),
            b"534D534E" => Ok(Self::Samsung),
            b"534E5300" => Ok(Self::Sinosun),
            b"534D5343" => Ok(Self::SMSC),
            b"53544D20" => Ok(Self::StMicroelectronics),
            b"54584E00" => Ok(Self::TexasInstruments),
            b"57454300" => Ok(Self::Winbond),
            _ => Err(WebauthnError::ParseNOMFailure),
        }
    }
}

// The result is concatenated together to form an 8 character name which is
// appended after the lower-case ASCII characters “id:”.
pub(crate) fn tpm_device_attribute_parser(i: &[u8]) -> nom::IResult<&[u8], &[u8; 8]> {
    let (i, _) = tag("id:")(i)?;
    let (i, vendor_code) = take(8usize)(i)?;

    Ok((i, vendor_code.try_into().expect("took exactly 8 bytes")))
}

impl TryFrom<&[u8]> for TpmVendor {
    type Error = WebauthnError;
    fn try_from(data: &[u8]) -> Result<Self, Self::Error> {
        tpm_device_attribute_parser(data)
            .map_err(|e| {
                error!(?e, "try_from tpm_device_attribute_parser");
                WebauthnError::ParseNOMFailure
            })
            .and_then(|(_, v)| TpmVendor::try_from(v))
    }
}

#[cfg(test)]
mod tests {
    use super::{
        AttestationObject, Authentication, AuthenticatorData, CredentialProtectionPolicy,
        RegisterPublicKeyCredential, Registration, RegistrationSignedExtensions, TpmsAttest,
        TpmtPublic, TpmtSignature, TPM_GENERATED_VALUE,
    };
    use crate::interface::*;
    use base64::{engine::general_purpose::STANDARD, Engine};

    #[test]
    fn deserialise_register_response() {
        let x = r#"
        {   "id":"4oiUggKcrpRIlB-cFzFbfkx_BNeM7UAnz3wO7ZpT4I2GL_n-g8TICyJTHg11l0wyc-VkQUVnJ0yM08-1D5oXnw",
            "rawId":"4oiUggKcrpRIlB-cFzFbfkx_BNeM7UAnz3wO7ZpT4I2GL_n-g8TICyJTHg11l0wyc-VkQUVnJ0yM08-1D5oXnw",
            "response":{
                "attestationObject":"o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVjEEsoXtJryKJQ28wPgFmAwoh5SXSZuIJJnQzgBqP1AcaBBAAAAAAAAAAAAAAAAAAAAAAAAAAAAQOKIlIICnK6USJQfnBcxW35MfwTXjO1AJ898Du2aU-CNhi_5_oPEyAsiUx4NdZdMMnPlZEFFZydMjNPPtQ-aF5-lAQIDJiABIVggFo08FM4Je1yfCSuPsxP6h0zvlJSjfocUk75EvXw2oSMiWCArRwLD8doar0bACWS1PgVJKzp_wStyvOkTd4NlWHW8rQ",
                "clientDataJSON":"eyJjaGFsbGVuZ2UiOiJwZENXRDJWamRMSVkzN2VSYTVfazdhS3BqdkF2VmNOY04ycVozMjk0blpVIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cDovLzEyNy4wLjAuMTo4MDgwIiwidHlwZSI6IndlYmF1dGhuLmNyZWF0ZSJ9"
            },
            "type":"public-key"
        }
        "#;
        let _y: RegisterPublicKeyCredential = serde_json::from_str(x).unwrap();
    }

    #[test]
    fn deserialise_attestation_object() {
        let raw_ao = STANDARD.decode(
            "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVjEEsoXtJryKJQ28wPgFmAwoh5SXSZuIJJnQzgBqP1AcaBBAAAAAAAAAAAAAAAAAAAAAAAAAAAAQCgxaVISCxE+DrcxP5/+aPM88CTI+04J+o61SK6mnepjGZYv062AbtydzWmbAxF00VSAyp0ImP94uoy+0y7w9yilAQIDJiABIVggGT9woA+UoX+jBxuiHQpdkm0kCVh75WTj3TXl4zLJuzoiWCBKiCneKgWJgWiwrZedNwl06GTaXyaGrYS4bPbBraInyg=="
        ).unwrap();

        let _ao = AttestationObject::<Registration>::try_from(raw_ao.as_slice()).unwrap();
    }

    // Add tests for when the objects are too short.
    //
    #[test]
    fn deserialise_tpms_attest() {
        let data: Vec<u8> = vec![
            255, 84, 67, 71, // magic
            128, 23, // type_
            0, 34, // 2b_name size
            0, 11, 174, 74, 152, 70, 1, 87, 191, 156, 96, 74, 177, 221, 37, 132, 6, 8, 101, 35,
            124, 216, 85, 173, 85, 195, 115, 137, 194, 247, 145, 61, 82, 40, // 2b_name data
            0, 20, // exdata size
            234, 98, 144, 49, 146, 39, 99, 47, 44, 82, 115, 48, 64, 40, 152, 224, 227, 42, 63,
            133, // ext data
            0, 0, 0, 2, 219, 215, 137, 38, // clock
            187, 106, 183, 8, // reset
            100, 145, 106, 200, // restart
            1,   // safe
            86, 5, 220, 81, 118, 234, 131, 141, // fw vers
            0, 34, // type attested.
            0, 11, 239, 53, 112, 255, 253, 12, 189, 168, 16, 253, 10, 149, 108, 7, 31, 212, 143,
            21, 153, 7, 7, 153, 99, 73, 205, 97, 90, 110, 182, 120, 4, 250, 0, 34, 0, 11, 249, 72,
            224, 84, 16, 96, 147, 197, 167, 195, 110, 181, 77, 207, 147, 16, 34, 64, 139, 185, 120,
            190, 196, 209, 213, 29, 1, 136, 76, 235, 223, 247,
        ];

        let tpms_attest = TpmsAttest::try_from(data.as_slice()).unwrap();
        println!("{tpms_attest:?}");
        assert!(tpms_attest.magic == TPM_GENERATED_VALUE);
    }

    #[test]
    fn deserialise_tpmt_public() {
        // The TPMT_PUBLIC structure (see [TPMv2-Part2] section 12.2.4) used by the TPM to represent the credential public key.
        let data: Vec<u8> = vec![
            0, 1, 0, 11, 0, 6, 4, 114, 0, 32, 157, 255, 203, 243, 108, 56, 58, 230, 153, 251, 152,
            104, 220, 109, 203, 137, 215, 21, 56, 132, 190, 40, 3, 146, 44, 18, 65, 88, 191, 173,
            34, 174, 0, 16, 0, 16, 8, 0, 0, 0, 0, 0, 1, 0, 220, 20, 243, 114, 251, 142, 90, 236,
            17, 204, 181, 223, 8, 72, 230, 209, 122, 44, 90, 55, 96, 134, 69, 16, 125, 139, 112,
            81, 154, 230, 133, 211, 129, 37, 75, 208, 222, 70, 210, 239, 209, 188, 152, 93, 222,
            222, 154, 169, 217, 160, 90, 243, 135, 151, 25, 87, 240, 178, 106, 119, 150, 89, 23,
            223, 158, 88, 107, 72, 101, 61, 184, 132, 19, 110, 144, 107, 22, 178, 252, 206, 50,
            207, 11, 177, 137, 35, 139, 68, 212, 148, 121, 249, 50, 35, 89, 52, 47, 26, 23, 6, 15,
            115, 155, 127, 59, 168, 208, 196, 78, 125, 205, 0, 98, 43, 223, 233, 65, 137, 103, 2,
            227, 35, 81, 107, 247, 230, 186, 111, 27, 4, 57, 42, 220, 32, 29, 181, 159, 6, 176,
            182, 94, 191, 222, 212, 235, 60, 101, 83, 86, 217, 203, 151, 251, 254, 219, 204, 195,
            10, 74, 147, 5, 27, 167, 127, 117, 149, 245, 157, 92, 124, 2, 196, 214, 107, 246, 228,
            171, 229, 100, 212, 67, 88, 215, 75, 33, 183, 199, 51, 171, 210, 213, 65, 45, 96, 96,
            226, 29, 130, 254, 58, 92, 252, 133, 207, 105, 63, 156, 208, 149, 142, 9, 83, 1, 193,
            217, 244, 35, 137, 43, 138, 137, 140, 82, 231, 195, 145, 213, 230, 185, 245, 104, 105,
            62, 142, 124, 34, 9, 157, 167, 188, 243, 112, 104, 248, 63, 50, 19, 53, 173, 69, 12,
            39, 252, 9, 69, 223,
        ];
        let tpmt_public = TpmtPublic::try_from(data.as_slice()).unwrap();
        println!("{tpmt_public:?}");
    }

    #[test]
    fn deserialise_tpmt_signature() {
        // The attestation signature, in the form of a TPMT_SIGNATURE structure as specified in [TPMv2-Part2] section 11.3.4.
        let data: Vec<u8> = vec![
            5, 3, 162, 216, 151, 57, 210, 103, 145, 121, 161, 186, 63, 232, 221, 255, 89, 37, 17,
            59, 155, 241, 77, 30, 35, 201, 30, 140, 84, 214, 250, 185, 47, 248, 58, 89, 177, 187,
            231, 202, 220, 45, 167, 126, 243, 194, 94, 33, 39, 205, 163, 51, 40, 171, 35, 118, 196,
            244, 247, 143, 166, 193, 223, 94, 244, 157, 121, 220, 22, 94, 163, 15, 151, 223, 214,
            131, 105, 202, 40, 16, 176, 11, 154, 102, 100, 212, 174, 103, 166, 92, 90, 154, 224,
            20, 165, 106, 127, 53, 91, 230, 217, 199, 172, 195, 203, 242, 41, 158, 64, 252, 65, 9,
            155, 160, 63, 40, 94, 94, 64, 145, 173, 71, 85, 173, 2, 199, 18, 148, 88, 223, 93, 154,
            203, 197, 170, 142, 35, 249, 146, 107, 146, 2, 14, 54, 39, 151, 181, 10, 176, 216, 117,
            25, 196, 2, 205, 159, 140, 155, 56, 89, 87, 31, 135, 93, 97, 78, 95, 176, 228, 72, 237,
            130, 171, 23, 66, 232, 35, 115, 218, 105, 168, 6, 253, 121, 161, 129, 44, 78, 252, 44,
            11, 23, 172, 66, 37, 214, 113, 128, 28, 33, 209, 66, 34, 32, 196, 153, 80, 87, 243,
            162, 7, 25, 62, 252, 243, 174, 31, 168, 98, 123, 100, 2, 143, 134, 36, 154, 236, 18,
            128, 175, 185, 189, 177, 51, 53, 216, 190, 43, 63, 35, 84, 14, 64, 249, 23, 9, 125,
            147, 160, 176, 137, 30, 174, 245, 148, 189,
        ];
        let tpmt_sig = TpmtSignature::try_from(data.as_slice()).unwrap();
        println!("{tpmt_sig:?}");
    }

    #[test]
    fn deserialize_extensions() {
        let data: Vec<u8> = vec![
            161, 107, 99, 114, 101, 100, 80, 114, 111, 116, 101, 99, 116, 3,
        ];

        let extensions: RegistrationSignedExtensions = serde_cbor_2::from_slice(&data).unwrap();

        let cred_protect = extensions
            .cred_protect
            .expect("should have cred protect extension");
        println!("{cred_protect:?}");
        assert_eq!(
            cred_protect.0,
            CredentialProtectionPolicy::UserVerificationRequired
        );
    }

    #[test]
    fn credential_protection_policy_conversions() {
        use CredentialProtectionPolicy::*;
        assert_eq!(
            UserVerificationOptional,
            CredentialProtectionPolicy::try_from(UserVerificationOptional as u8).unwrap()
        );
        assert_eq!(
            UserVerificationOptionalWithCredentialIDList,
            CredentialProtectionPolicy::try_from(
                UserVerificationOptionalWithCredentialIDList as u8
            )
            .unwrap()
        );
        assert_eq!(
            UserVerificationRequired,
            CredentialProtectionPolicy::try_from(UserVerificationRequired as u8).unwrap()
        );
    }

    #[test]
    fn authenticator_data_parser_extensions() {
        let _ = tracing_subscriber::fmt::try_init();
        let raw = [
            73, 150, 13, 229, 136, 14, 140, 104, 116, 52, 23, 15, 100, 118, 96, 91, 143, 228, 174,
            185, 162, 134, 50, 199, 153, 92, 243, 186, 131, 29, 151, 99, 133, 0, 0, 0, 6, 161, 104,
            99, 114, 101, 100, 66, 108, 111, 98, 64,
        ];

        assert!(AuthenticatorData::<Authentication>::try_from(raw.as_slice()).is_ok());
    }

    #[test]
    fn migrate_credentialv3() {
        let legacy_cred = r#"{"cred_id":[185,151,21,12,21,82,235,193,63,50,208,32,121,10,68,148,156,101,116,95,250,113,143,108,74,246,214,171,31,234,70,31,48,138,238,54,151,36,65,70,104,121,200,87,131,254,191,100,215,125,29,49,177,71,4,114,61,69,49,96,116,148,8,205],"cred":{"type_":"ES256","key":{"EC_EC2":{"curve":"SECP256R1","x":[194,126,127,109,252,23,131,21,252,6,223,99,44,254,140,27,230,17,94,5,133,28,104,41,144,69,171,149,161,26,200,243],"y":[143,123,183,156,24,178,21,248,117,159,162,69,171,52,188,252,26,59,6,47,103,92,19,58,117,103,249,0,219,8,95,196]}}},"counter":2,"verified":false,"registration_policy":"preferred"}"#;

        let cred: CredentialV3 = serde_json::from_str(legacy_cred).unwrap();

        println!("{cred:?}");

        let cred_migrated: Credential = cred.into();

        println!("{cred_migrated:?}");
    }
}
