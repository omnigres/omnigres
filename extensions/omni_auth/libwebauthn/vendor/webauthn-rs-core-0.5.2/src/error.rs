//! Possible errors that may occur during Webauthn Operation processing

use base64::DecodeError as b64DecodeError;
use openssl::error::ErrorStack as OpenSSLErrorStack;
use serde_cbor_2::error::Error as CBORError;
use serde_json::error::Error as JSONError;
// use serde::{Deserialize, Serialize};
// use nom::Err as NOMError;

/// A wrapper for `Result<T, WebauthnError>`
pub type WebauthnResult<T> = core::result::Result<T, WebauthnError>;

/// Possible errors that may occur during Webauthn Operation processing.
#[derive(Debug, thiserror::Error)]
#[allow(missing_docs)]
pub enum WebauthnError {
    #[error("The configuration was invalid")]
    Configuration,

    #[error("The JSON from the client did not indicate webauthn.<method> correctly")]
    InvalidClientDataType,

    #[error(
        "The client response challenge differs from the latest challenge issued to the userId"
    )]
    MismatchedChallenge,

    #[error("There are no challenges associated to the UserId")]
    ChallengeNotFound,

    #[error("The clients relying party origin does not match our servers information")]
    InvalidRPOrigin,

    #[error("The clients relying party id hash does not match the hash of our relying party id")]
    InvalidRPIDHash,

    #[error("The user present bit is not set, and required")]
    UserNotPresent,

    #[error("The user verified bit is not set, and required by policy")]
    UserNotVerified,

    #[error("The extensions are unknown to this server")]
    InvalidExtensions,

    #[error("An extension for this identifier was not in the authenticator data")]
    AuthenticatorDataMissingExtension,

    #[error("The required attestation data is not present in the response")]
    MissingAttestationCredentialData,

    #[error("The attestation format requested is not able to be processed by this server - please report an issue to add the attestation format")]
    AttestationNotSupported,

    #[error("A failure occurred in persisting the Challenge data")]
    ChallengePersistenceError,

    #[error("The attestation statement map is not valid")]
    AttestationStatementMapInvalid,

    #[error("The attestation statement response is not present")]
    AttestationStatementResponseMissing,

    #[error("The attestation statement response is not valid")]
    AttestationStatementResponseInvalid,

    #[error("The attestation statement signature is not present")]
    AttestationStatementSigMissing,

    #[error("The attestation statement signature is not valid")]
    AttestationStatementSigInvalid,

    #[error("The attestation statement version is not present")]
    AttestationStatementVerMissing,

    #[error("The attestation statement version is not valid")]
    AttestationStatementVerInvalid,

    #[error("The attestation statement version not supported")]
    AttestationStatementVerUnsupported,

    #[error("The attestation statement x5c (trust root) is not present")]
    AttestationStatementX5CMissing,

    #[error("The attestation statement x5c (trust root) is not valid")]
    AttestationStatementX5CInvalid,

    #[error("The attestation statement algorithm is not present")]
    AttestationStatementAlgMissing,

    #[error("The attestation statement certInfo is not present")]
    AttestationStatementCertInfoMissing,

    #[error("A required extension was not in the attestation statement")]
    AttestationStatementMissingExtension,

    #[error("The attestation statement pubArea is not present")]
    AttestationStatementPubAreaMissing,

    #[error("The attestation statement alg does not match algorithm of the credentialPublicKey in authenticatorData")]
    AttestationStatementAlgMismatch,

    #[error("The attestation statement alg does not match algorithm of the credentialPublicKey in authenticatorData")]
    AttestationStatementAlgInvalid,

    #[error("The attestation trust could not be established")]
    AttestationTrustFailure,

    #[error("The attestation Certificate's OID 1.3.6.1.4.1.45724.1.1.4 aaguid does not match the aaguid of the token")]
    AttestationCertificateAAGUIDMismatch,

    #[error("The attestation Certificate's OID 1.2.840.113635.100.8.2 value does not match the computed nonce")]
    AttestationCertificateNonceMismatch,

    #[error("The attestation created by the TPM is not correct")]
    AttestationTpmStInvalid,

    #[error("The TPM attestation and key algorithms do not match")]
    AttestationTpmPubAreaMismatch,

    #[error("The TPM attestation extraData is missing or invalid")]
    AttestationTpmExtraDataInvalid,

    #[error("The TPM attestation extraData does not match the hash of the verification data")]
    AttestationTpmExtraDataMismatch,

    #[error("The TPM requested hash over pubArea is unknown")]
    AttestationTpmPubAreaHashUnknown,

    #[error("The TPM requested hash over pubArea is invalid")]
    AttestationTpmPubAreaHashInvalid,

    #[error("The TPM attest certify structure is invalid")]
    AttestationTpmAttestCertifyInvalid,

    #[error("The requirements of https://w3c.github.io/webauthn/#sctn-packed-attestation-cert-requirements are not met by this attestation certificate")]
    AttestationCertificateRequirementsNotMet,

    #[error("The provided list of CA's for attestation is empty, allowing no trust path to be established")]
    AttestationCertificateTrustStoreEmpty,

    #[error("The leaf certificate we intented to verify is missing.")]
    AttestationLeafCertMissing,

    #[error("The attestation was parsed, but is not a format valid for CA chain validation")]
    AttestationNotVerifiable,

    #[error("The attestation CA that was trusted limits the aaguids allowed, this device is not a member of that set")]
    AttestationUntrustedAaguid,

    #[error("The attestation CA that was trusted limits the aaguids allowed, but this device does not have an aaguid")]
    AttestationFormatMissingAaguid,

    #[error(
        "The attestation was parsed, but is not trusted by one of the selected CA certificates"
    )]
    AttestationChainNotTrusted(String),

    #[error("The X5C trust root is not a valid algorithm for signing")]
    CertificatePublicKeyInvalid,

    #[error("A base64 parser failure has occurred")]
    ParseBase64Failure(#[from] b64DecodeError),

    #[error("A CBOR parser failure has occurred")]
    ParseCBORFailure(#[from] CBORError),

    #[error("A JSON parser failure has occurred")]
    ParseJSONFailure(#[from] JSONError),

    #[error("A NOM parser failure has occurred")]
    ParseNOMFailure,

    #[error("In parsing the attestation object, there was insufficient data")]
    ParseInsufficientBytesAvailable,

    #[error("An OpenSSL Error has occurred")]
    OpenSSLError(#[from] OpenSSLErrorStack),

    #[error("The requested OpenSSL curve is not supported by OpenSSL")]
    OpenSSLErrorNoCurveName,

    #[error("The COSEKey contains invalid CBOR which can not be processed")]
    COSEKeyInvalidCBORValue,

    #[error("The COSEKey type is not supported by this implementation")]
    COSEKeyInvalidType,

    #[error("ED25519 and ED448 keys are not supported by this implementation")]
    COSEKeyEDUnsupported,

    #[error("The COSEKey contains invalid ECDSA X/Y coordinate data")]
    COSEKeyECDSAXYInvalid,

    #[error("The COSEKey contains invalid RSA modulus/exponent data")]
    COSEKeyRSANEInvalid,

    #[error("The COSEKey uses a curve that is not supported by this implementation")]
    COSEKeyECDSAInvalidCurve,

    #[error("The COSEKey contains invalid EDDSA X coordinate data")]
    COSEKeyEDDSAXInvalid,

    #[error("The COSEKey uses a curve that is not supported by this implementation")]
    COSEKeyEDDSAInvalidCurve,

    #[error("The COSEKey contains invalid cryptographic algorithm request")]
    COSEKeyInvalidAlgorithm,

    #[error("The credential may be a passkey and not truly bound to hardware.")]
    CredentialMayNotBeHardwareBound,

    #[error("The credential uses insecure cryptographic routines and is not trusted")]
    CredentialInsecureCryptography,

    #[error("The credential exist check failed")]
    CredentialExistCheckError,

    #[error("The credential already exists")]
    CredentialAlreadyExists,

    #[error("The credential was not able to be persisted")]
    CredentialPersistenceError,

    #[error("The credential was not able to be retrieved")]
    CredentialRetrievalError,

    #[error("The credential requested could not be found")]
    CredentialNotFound,

    #[error("A credential alg that was not allowed in the request was attempted.")]
    CredentialAlteredAlgFromRequest,

    #[error("A credential that was excluded in the request attempted to register.")]
    CredentialExcludedFromRequest,

    #[error("The credential may have be compromised and should be inspected")]
    CredentialPossibleCompromise,

    #[error("The credential counter could not be updated")]
    CredentialCounterUpdateFailure,

    #[error("The provided call back failed to allow reporting the credential failure")]
    CredentialCompromiseReportFailure,

    #[error("The backup (passkey) elligibility of this device has changed, meaning it must be re-enrolled for security validation")]
    CredentialBackupElligibilityInconsistent,

    #[error("The trust path could not be established")]
    TrustFailure,

    #[error("Authentication has failed")]
    AuthenticationFailure,

    #[error("Inconsistent Credential Verification and User Verification Policy")]
    InconsistentUserVerificationPolicy,

    #[error("Invalid User Name supplied for registration")]
    InvalidUsername,

    #[error("Invalid UserID supplied during authentication")]
    InvalidUserUniqueId,

    #[error("Supplied Nid does not correspond to a supported ECDSA curve")]
    ECDSACurveInvalidNid,

    #[error("The attested credential public key and subject public key do not match")]
    AttestationCredentialSubjectKeyMismatch,

    #[error(
        "The credential was created in a cross-origin context (while cross-origin was disallowed)"
    )]
    CredentialCrossOrigin,

    #[error("The attestation ca list can not be empty")]
    MissingAttestationCaList,

    #[error("This key has an invalid backup state flag")]
    SshPublicKeyBackupState,

    #[error("ED25519 and ED448 keys are not supported by this implementation")]
    SshPublicKeyEDUnsupported,

    #[error("The requested ssh public key curve is invalid")]
    SshPublicKeyInvalidCurve,

    #[error("The SSH public key is invalid")]
    SshPublicKeyInvalidPubkey,

    #[error("The attestation requst indicates cred protect was required, but user verification was not performed")]
    SshPublicKeyInconsistentUserVerification,
}

impl PartialEq for WebauthnError {
    fn eq(&self, other: &Self) -> bool {
        std::mem::discriminant(self) == std::mem::discriminant(other)
    }
}
