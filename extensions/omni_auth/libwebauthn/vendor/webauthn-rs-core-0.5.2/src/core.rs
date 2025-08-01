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

#![warn(missing_docs)]

use rand::prelude::*;
use std::time::Duration;
use url::Url;

use crate::attestation::{
    verify_android_key_attestation, verify_apple_anonymous_attestation,
    verify_attestation_ca_chain, verify_fidou2f_attestation, verify_packed_attestation,
    verify_tpm_attestation,
};
use crate::constants::CHALLENGE_SIZE_BYTES;
use crate::crypto::compute_sha256;
use crate::error::WebauthnError;
use crate::internals::*;
use crate::proto::*;

/// The Core Webauthn handler.
///
/// It provides 4 interfaces methods for registering and then authenticating credentials.
/// * generate_challenge_register
/// * register_credential
/// * generate_challenge_authenticate
/// * authenticate_credential
///
/// Each of these is described in turn, but they will all map to routes in your application.
/// The generate functions return Json challenges that are intended to be processed by the client
/// browser, and the register and authenticate will receive Json that is processed and verified.
///
/// These functions return state that you must store and handle correctly for the authentication
/// or registration to proceed correctly.
///
/// As a result, it's very important you read the function descriptions to understand the process
/// as much as possible.
#[derive(Debug, Clone)]
pub struct WebauthnCore {
    rp_name: String,
    rp_id: String,
    rp_id_hash: [u8; 32],
    allowed_origins: Vec<Url>,
    authenticator_timeout: Duration,
    require_valid_counter_value: bool,
    #[allow(unused)]
    ignore_unsupported_attestation_formats: bool,
    allow_cross_origin: bool,
    allow_subdomains_origin: bool,
    allow_any_port: bool,
}

/// A builder allowing customisation of a client registration challenge.
pub struct ChallengeRegisterBuilder {
    user_unique_id: Vec<u8>,
    user_name: String,
    user_display_name: String,
    attestation: AttestationConveyancePreference,
    policy: UserVerificationPolicy,
    exclude_credentials: Option<Vec<CredentialID>>,
    extensions: Option<RequestRegistrationExtensions>,
    credential_algorithms: Vec<COSEAlgorithm>,
    require_resident_key: bool,
    authenticator_attachment: Option<AuthenticatorAttachment>,
    attestation_formats: Option<Vec<AttestationFormat>>,
    reject_synchronised_authenticators: bool,
    hints: Option<Vec<PublicKeyCredentialHints>>,
}

/// A builder allowing customisation of a client authentication challenge.
#[derive(Debug)]
pub struct ChallengeAuthenticateBuilder {
    creds: Vec<Credential>,
    policy: UserVerificationPolicy,
    extensions: Option<RequestAuthenticationExtensions>,
    allow_backup_eligible_upgrade: bool,
    hints: Option<Vec<PublicKeyCredentialHints>>,
}

impl ChallengeRegisterBuilder {
    /// Set the attestation conveyance preference. Defaults to None.
    pub fn attestation(mut self, value: AttestationConveyancePreference) -> Self {
        self.attestation = value;
        self
    }

    /// Request a user verification policy. Defaults to Preferred.
    pub fn user_verification_policy(mut self, value: UserVerificationPolicy) -> Self {
        self.policy = value;
        self
    }

    /// A list of credentials that the users agent will exclude from the registration.
    /// This is not a security control - it exists to help guide the user and the UI
    /// to avoid duplicate registration of credentials.
    pub fn exclude_credentials(mut self, value: Option<Vec<CredentialID>>) -> Self {
        self.exclude_credentials = value;
        self
    }

    /// Define extensions to be requested in the request. Defaults to None.
    pub fn extensions(mut self, value: Option<RequestRegistrationExtensions>) -> Self {
        self.extensions = value;
        self
    }

    /// Define the set of allowed credential algorithms. Defaults to the secure list.
    pub fn credential_algorithms(mut self, value: Vec<COSEAlgorithm>) -> Self {
        self.credential_algorithms = value;
        self
    }

    /// A flag to require that the authenticator must create a resident key. Defaults
    /// to false.
    pub fn require_resident_key(mut self, value: bool) -> Self {
        self.require_resident_key = value;
        self
    }

    /// Set the authenticator attachement preference. Defaults to None.
    pub fn authenticator_attachment(mut self, value: Option<AuthenticatorAttachment>) -> Self {
        self.authenticator_attachment = value;
        self
    }

    /// Flag that synchronised authenticators should not be allowed to register. Defaults to false.
    pub fn reject_synchronised_authenticators(mut self, value: bool) -> Self {
        self.reject_synchronised_authenticators = value;
        self
    }

    /// Add the set of hints for which public keys may satisfy this request.
    pub fn hints(mut self, hints: Option<Vec<PublicKeyCredentialHints>>) -> Self {
        self.hints = hints;
        self
    }

    /// Add the set of attestation formats that will be accepted in this operation
    pub fn attestation_formats(
        mut self,
        attestation_formats: Option<Vec<AttestationFormat>>,
    ) -> Self {
        self.attestation_formats = attestation_formats;
        self
    }
}

impl ChallengeAuthenticateBuilder {
    /// Define extensions to be requested in the request. Defaults to None.
    pub fn extensions(mut self, value: Option<RequestAuthenticationExtensions>) -> Self {
        self.extensions = value;
        self
    }

    /// Allow authenticators to modify their backup eligibility. This can occur
    /// where some formerly hardware bound devices become roaming ones. If in
    /// double leave this value as default (false, rejects credentials that
    /// post-create move from single device to roaming).
    pub fn allow_backup_eligible_upgrade(mut self, value: bool) -> Self {
        self.allow_backup_eligible_upgrade = value;
        self
    }

    /// Add the set of hints for which public keys may satisfy this request.
    pub fn hints(mut self, hints: Option<Vec<PublicKeyCredentialHints>>) -> Self {
        self.hints = hints;
        self
    }
}

impl WebauthnCore {
    /// ⚠️  ⚠️  ⚠️  THIS IS UNSAFE. AVOID USING THIS DIRECTLY ⚠️  ⚠️  ⚠️
    ///
    /// If possible, use the `webauthn-rs` crate, and it's safe wrapper instead!
    ///
    /// Webauthn as a standard has many traps that in the worst cases, may lead to
    /// bypasses and full account compromises. Many of the features of webauthn are
    /// NOT security policy, but user interface hints. Many options can NOT be
    /// enforced. `webauthn-rs` handles these correctly. USE `webauthn-rs` INSTEAD.
    ///
    /// If you still choose to continue, and use this directly, be aware that:
    ///
    /// * This function signature MAY change WITHOUT NOTICE and WITHIN MINOR VERSIONS
    /// * You MUST understand the webauthn specification in excruciating detail to understand the traps within it
    /// * That you are responsible for UPHOLDING many invariants within webauthn that are NOT DOCUMENTED in the webauthn specification
    ///
    /// Seriously. Use `webauthn-rs` instead.
    pub fn new_unsafe_experts_only(
        rp_name: &str,
        rp_id: &str,
        allowed_origins: Vec<Url>,
        authenticator_timeout: Duration,
        allow_subdomains_origin: Option<bool>,
        allow_any_port: Option<bool>,
    ) -> Self {
        let rp_id_hash = compute_sha256(rp_id.as_bytes());
        WebauthnCore {
            rp_name: rp_name.to_string(),
            rp_id: rp_id.to_string(),
            rp_id_hash,
            allowed_origins,
            authenticator_timeout,
            require_valid_counter_value: true,
            ignore_unsupported_attestation_formats: true,
            allow_cross_origin: false,
            allow_subdomains_origin: allow_subdomains_origin.unwrap_or(false),
            allow_any_port: allow_any_port.unwrap_or(false),
        }
    }

    /// Get the currently configured origins
    pub fn get_allowed_origins(&self) -> &[Url] {
        &self.allowed_origins
    }

    fn generate_challenge(&self) -> Challenge {
        let mut rng = rand::thread_rng();
        Challenge::new(rng.gen::<[u8; CHALLENGE_SIZE_BYTES]>().to_vec())
    }

    /// Generate a new challenge builder for client registration. This is the first step in
    /// the lifecycle of a credential. This function will return a register builder
    /// allowing you to customise the parameters that will be sent to the client.
    pub fn new_challenge_register_builder(
        &self,
        user_unique_id: &[u8],
        user_name: &str,
        user_display_name: &str,
    ) -> Result<ChallengeRegisterBuilder, WebauthnError> {
        if user_unique_id.is_empty() || user_display_name.is_empty() || user_name.is_empty() {
            return Err(WebauthnError::InvalidUsername);
        }

        Ok(ChallengeRegisterBuilder {
            user_unique_id: user_unique_id.to_vec(),
            user_name: user_name.to_string(),
            user_display_name: user_display_name.to_string(),
            credential_algorithms: COSEAlgorithm::secure_algs(),
            attestation: Default::default(),
            policy: UserVerificationPolicy::Preferred,
            exclude_credentials: Default::default(),
            extensions: Default::default(),
            require_resident_key: Default::default(),
            authenticator_attachment: Default::default(),
            reject_synchronised_authenticators: Default::default(),
            hints: Default::default(),
            attestation_formats: Default::default(),
        })
    }

    /// Generate a new challenge for client registration from the parameters defined by the
    /// `ChallengeRegisterBuilder`.
    ///
    /// This function will return the
    /// `CreationChallengeResponse` which is suitable for serde json serialisation
    /// to be sent to the client.
    /// The client (generally a web browser) will pass this JSON
    /// structure to the `navigator.credentials.create()` javascript function for registration.
    ///
    /// It also returns a RegistrationState, that you *must*
    /// persist. It is strongly advised you associate this RegistrationState with the
    /// UserId of the requester.
    #[allow(clippy::too_many_arguments)]
    pub fn generate_challenge_register(
        &self,
        challenge_builder: ChallengeRegisterBuilder,
    ) -> Result<(CreationChallengeResponse, RegistrationState), WebauthnError> {
        let ChallengeRegisterBuilder {
            user_unique_id,
            user_name,
            user_display_name,
            attestation,
            policy,
            exclude_credentials,
            extensions,
            credential_algorithms,
            require_resident_key,
            authenticator_attachment,
            attestation_formats,
            reject_synchronised_authenticators,
            hints,
        } = challenge_builder;

        let challenge = self.generate_challenge();

        let resident_key = if require_resident_key {
            Some(ResidentKeyRequirement::Required)
        } else {
            Some(ResidentKeyRequirement::Discouraged)
        };

        let timeout_millis =
            u32::try_from(self.authenticator_timeout.as_millis()).expect("Timeout too large");

        let c = CreationChallengeResponse {
            public_key: PublicKeyCredentialCreationOptions {
                rp: RelyingParty {
                    name: self.rp_name.clone(),
                    id: self.rp_id.clone(),
                },
                user: User {
                    id: user_unique_id.into(),
                    name: user_name,
                    display_name: user_display_name,
                },
                challenge: challenge.clone().into(),
                pub_key_cred_params: credential_algorithms
                    .iter()
                    .map(|alg| PubKeyCredParams {
                        type_: "public-key".to_string(),
                        alg: *alg as i64,
                    })
                    .collect(),
                timeout: Some(timeout_millis),
                hints,
                attestation: Some(attestation),
                exclude_credentials: exclude_credentials.as_ref().map(|creds| {
                    creds
                        .iter()
                        .cloned()
                        .map(|id| PublicKeyCredentialDescriptor {
                            type_: "public-key".to_string(),
                            id: id.as_ref().into(),
                            transports: None,
                        })
                        .collect()
                }),
                authenticator_selection: Some(AuthenticatorSelectionCriteria {
                    authenticator_attachment,
                    resident_key,
                    require_resident_key,
                    user_verification: policy,
                }),
                extensions: extensions.clone(),
                attestation_formats,
            },
        };

        let wr = RegistrationState {
            policy,
            exclude_credentials: exclude_credentials.unwrap_or_else(|| Vec::with_capacity(0)),
            challenge: challenge.into(),
            credential_algorithms,
            // We can potentially enforce these!
            require_resident_key,
            authenticator_attachment,
            extensions: extensions.unwrap_or_default(),
            allow_synchronised_authenticators: !reject_synchronised_authenticators,
        };

        // This should have an opaque type of username + chal + policy
        Ok((c, wr))
    }

    /// Process a credential registration response. This is the output of
    /// `navigator.credentials.create()` which is sent to the webserver from the client.
    ///
    /// Given the username you also must provide the associated RegistrationState for this
    /// operation to proceed.
    ///
    /// On success this returns a new Credential that you must persist and associate with the
    /// user.
    ///
    /// You need to provide a closure that is able to check if any credential of the
    /// same id has already been persisted by your server.
    pub fn register_credential(
        &self,
        reg: &RegisterPublicKeyCredential,
        state: &RegistrationState,
        attestation_cas: Option<&AttestationCaList>,
        // does_exist_fn: impl Fn(&CredentialID) -> Result<bool, ()>,
    ) -> Result<Credential, WebauthnError> {
        // Decompose our registration state which contains everything we need to proceed.
        trace!(?state);
        trace!(?reg);

        let RegistrationState {
            policy,
            exclude_credentials,
            challenge,
            credential_algorithms,
            require_resident_key: _,
            authenticator_attachment: _,
            extensions,
            allow_synchronised_authenticators,
        } = state;
        let chal: &ChallengeRef = challenge.into();

        // send to register_credential_internal
        let credential = self.register_credential_internal(
            reg,
            *policy,
            chal,
            exclude_credentials,
            credential_algorithms,
            attestation_cas,
            false,
            extensions,
            *allow_synchronised_authenticators,
        )?;

        // Check that the credentialId is not yet registered to any other user. If registration is
        // requested for a credential that is already registered to a different user, the Relying
        // Party SHOULD fail this registration ceremony, or it MAY decide to accept the registration,
        // e.g. while deleting the older registration.

        /*
        let cred_exist_result = does_exist_fn(&credential.0.cred_id)
            .map_err(|_| WebauthnError::CredentialExistCheckError)?;

        if cred_exist_result {
            return Err(WebauthnError::CredentialAlreadyExists);
        }
        */

        Ok(credential)
    }

    #[allow(clippy::too_many_arguments)]
    pub(crate) fn register_credential_internal(
        &self,
        reg: &RegisterPublicKeyCredential,
        policy: UserVerificationPolicy,
        chal: &ChallengeRef,
        exclude_credentials: &[CredentialID],
        credential_algorithms: &[COSEAlgorithm],
        attestation_cas: Option<&AttestationCaList>,
        danger_disable_certificate_time_checks: bool,
        req_extn: &RequestRegistrationExtensions,
        allow_synchronised_authenticators: bool,
    ) -> Result<Credential, WebauthnError> {
        // Internal management - if the attestation ca list is some, but is empty, we need to fail!
        if attestation_cas
            .as_ref()
            .map(|l| l.is_empty())
            .unwrap_or(false)
        {
            return Err(WebauthnError::MissingAttestationCaList);
        }

        // ======================================================================
        // References:
        // Level 2: https://www.w3.org/TR/webauthn-2/#sctn-registering-a-new-credential

        // Steps 1 through 4 are performed by the Client, not the RP.

        // Let JSONtext be the result of running UTF-8 decode on the value of response.clientDataJSON.
        //
        //   This is done by the calling webserver to give us reg aka JSONText in this case.
        //
        // Let C, the client data claimed as collected during the credential creation, be the result
        // of running an implementation-specific JSON parser on JSONtext.
        //
        // Now, we actually do a much larger conversion in one shot
        // here, where we get the AuthenticatorAttestationResponse

        let data = AuthenticatorAttestationResponse::try_from(&reg.response)?;

        // trace!("data: {:?}", data);

        // Verify that the value of C.type is webauthn.create.
        if data.client_data_json.type_ != "webauthn.create" {
            return Err(WebauthnError::InvalidClientDataType);
        }

        // Verify that the value of C.challenge matches the challenge that was sent to the
        // authenticator in the create() call.
        if data.client_data_json.challenge.as_slice() != chal.as_ref() {
            return Err(WebauthnError::MismatchedChallenge);
        }

        // Verify that the client's origin matches one of our allowed origins..
        if !self.allowed_origins.iter().any(|origin| {
            Self::origins_match(
                self.allow_subdomains_origin,
                self.allow_any_port,
                &data.client_data_json.origin,
                origin,
            )
        }) {
            return Err(WebauthnError::InvalidRPOrigin);
        }

        // ATM most browsers do not send this value, so we must default to
        // `false`. See [WebauthnConfig::allow_cross_origin] doc-comment for
        // more.
        if !self.allow_cross_origin && data.client_data_json.cross_origin.unwrap_or(false) {
            return Err(WebauthnError::CredentialCrossOrigin);
        }

        // Verify that the value of C.tokenBinding.status matches the state of Token Binding for the
        // TLS connection over which the assertion was obtained. If Token Binding was used on that
        // TLS connection, also verify that C.tokenBinding.id matches the base64url encoding of the
        // Token Binding ID for the connection.
        //
        //  This could be reasonably complex to do, given that we could be behind a load balancer
        // or we may not directly know the status of TLS inside this api. I'm open to creative
        // suggestions on this topic!
        //

        // Let hash be the result of computing a hash over response.clientDataJSON using SHA-256
        //
        //   clarifying point - this is the hash of bytes.
        //
        // First you have to decode this from base64!!! This really could just be implementation
        // specific though ...
        let client_data_json_hash = compute_sha256(data.client_data_json_bytes.as_slice());

        // Perform CBOR decoding on the attestationObject field of the AuthenticatorAttestationResponse
        // structure to obtain the attestation statement format fmt, the authenticator data authData,
        // and the attestation statement attStmt.
        //
        // Done as part of try_from

        // Verify that the rpIdHash in authData is the SHA-256 hash of the RP ID expected by the
        // Relying Party.
        //
        //  NOW: Remember that RP ID https://w3c.github.io/webauthn/#rp-id is NOT THE SAME as the RP name
        // it's actually derived from the RP origin.
        if data.attestation_object.auth_data.rp_id_hash != self.rp_id_hash {
            return Err(WebauthnError::InvalidRPIDHash);
        }

        // Verify that the User Present bit of the flags in authData is set.
        if !data.attestation_object.auth_data.user_present {
            return Err(WebauthnError::UserNotPresent);
        }

        // TODO: Is it possible to verify the attachement policy and resident
        // key requirement here?

        // If user verification is required for this registration, verify that the User Verified bit
        // of the flags in authData is set.
        if matches!(policy, UserVerificationPolicy::Required)
            && !data.attestation_object.auth_data.user_verified
        {
            return Err(WebauthnError::UserNotVerified);
        }

        // Verify that the "alg" parameter in the credential public key in authData matches the alg
        // attribute of one of the items in options.pubKeyCredParams.
        //
        // WARNING: This is actually done after attestation as the credential public key
        // is NOT available yet!

        // Verify that the values of the client extension outputs in clientExtensionResults and the
        // authenticator extension outputs in the extensions in authData are as expected,
        // considering the client extension input values that were given as the extensions option in
        // the create() call. In particular, any extension identifier values in the
        // clientExtensionResults and the extensions in authData MUST be also be present as
        // extension identifier values in the extensions member of options, i.e., no extensions are
        // present that were not requested. In the general case, the meaning of "are as expected" is
        // specific to the Relying Party and which extensions are in use.

        debug!(
            "extensions: {:?}",
            data.attestation_object.auth_data.extensions
        );

        // Only packed, tpm and apple allow extensions to be verified!

        // Determine the attestation statement format by performing a USASCII case-sensitive match on
        // fmt against the set of supported WebAuthn Attestation Statement Format Identifier values.
        // An up-to-date list of registered WebAuthn Attestation Statement Format Identifier values
        // is maintained in the IANA registry of the same name [WebAuthn-Registries].
        // ( https://www.rfc-editor.org/rfc/rfc8809 )
        //
        //  https://w3c.github.io/webauthn-3/#packed-attestation
        //  https://w3c.github.io/webauthn-3/#tpm-attestation
        //  https://w3c.github.io/webauthn-3/#android-key-attestation
        //  https://w3c.github.io/webauthn-3/#android-safetynet-attestation
        //  https://w3c.github.io/webauthn-3/#fido-u2f-attestation
        //  https://w3c.github.io/webauthn-3/#none-attestation
        //  https://www.w3.org/TR/webauthn-3/#sctn-apple-anonymous-attestation
        //
        let attest_format = AttestationFormat::try_from(data.attestation_object.fmt.as_str())
            .map_err(|()| WebauthnError::AttestationNotSupported)?;

        // Verify that attStmt is a correct attestation statement, conveying a valid attestation
        // signature, by using the attestation statement format fmt’s verification procedure given
        // attStmt, authData and the hash of the serialized client data.

        let acd = data
            .attestation_object
            .auth_data
            .acd
            .as_ref()
            .ok_or(WebauthnError::MissingAttestationCredentialData)?;

        // Now, match based on the attest_format
        debug!("attestation is: {:?}", &attest_format);
        debug!("attested credential data is: {:?}", &acd);

        let (attestation_data, attestation_metadata) = match attest_format {
            AttestationFormat::FIDOU2F => (
                verify_fidou2f_attestation(acd, &data.attestation_object, &client_data_json_hash)?,
                AttestationMetadata::None,
            ),
            AttestationFormat::Packed => {
                verify_packed_attestation(acd, &data.attestation_object, &client_data_json_hash)?
            }
            // AttestationMetadata::None,
            AttestationFormat::Tpm => {
                verify_tpm_attestation(acd, &data.attestation_object, &client_data_json_hash)?
            }
            // AttestationMetadata::None,
            AttestationFormat::AppleAnonymous => verify_apple_anonymous_attestation(
                acd,
                &data.attestation_object,
                &client_data_json_hash,
            )?,
            // AttestationMetadata::None,
            AttestationFormat::AndroidKey => verify_android_key_attestation(
                acd,
                &data.attestation_object,
                &client_data_json_hash,
            )?,
            AttestationFormat::AndroidSafetyNet => {
                return Err(WebauthnError::AttestationNotSupported)
            }
            AttestationFormat::None => (ParsedAttestationData::None, AttestationMetadata::None),
        };

        let credential: Credential = Credential::new(
            acd,
            &data.attestation_object.auth_data,
            COSEKey::try_from(&acd.credential_pk)?,
            policy,
            ParsedAttestation {
                data: attestation_data,
                metadata: attestation_metadata,
            },
            req_extn,
            &reg.extensions,
            attest_format,
            &data.transports,
        );

        // Now based on result ...

        // If validation is successful, obtain a list of acceptable trust anchors (attestation
        // root certificates or ECDAA-Issuer public keys) for that attestation type and attestation
        // statement format fmt, from a trusted source or from policy. For example, the FIDO Metadata
        // Service [FIDOMetadataService] provides one way to obtain such information, using the
        // aaguid in the attestedCredentialData in authData.
        //
        // Assess the attestation trustworthiness using the outputs of the verification procedure in step 19, as follows:
        //
        // * If no attestation was provided, verify that None attestation is acceptable under Relying Party policy.
        // * If self attestation was used, verify that self attestation is acceptable under Relying Party policy.
        // * Otherwise, use the X.509 certificates returned as the attestation trust path from the verification
        //     procedure to verify that the attestation public key either correctly chains up to an acceptable
        //     root certificate, or is itself an acceptable certificate (i.e., it and the root certificate
        //     obtained in Step 20 may be the same).

        // If the attestation statement attStmt successfully verified but is not trustworthy per step 21 above,
        // the Relying Party SHOULD fail the registration ceremony.

        let attested_ca_crt = if let Some(ca_list) = attestation_cas {
            // If given a set of ca's assert that our attestation actually matched one.
            let ca_crt = verify_attestation_ca_chain(
                &credential.attestation.data,
                ca_list,
                danger_disable_certificate_time_checks,
            )?;

            // It may seem odd to unwrap the option and make this not verified at this point,
            // but in this case because we have the ca_list and none was the result (which happens)
            // in some cases, we need to map that through. But we need verify_attesation_ca_chain
            // to still return these option types due to re-attestation in the future.
            let ca_crt = ca_crt.ok_or_else(|| {
                warn!("device attested with a certificate not present in attestation ca chain");
                WebauthnError::AttestationNotVerifiable
            })?;
            Some(ca_crt)
        } else {
            None
        };

        debug!("attested_ca_crt = {:?}", attested_ca_crt);

        // Assert that the aaguid of the device, is within the authority of this CA (if
        // a list of aaguids was provided, and the ca blanket allows verification).
        if let Some(att_ca_crt) = attested_ca_crt {
            if att_ca_crt.blanket_allow() {
                trace!("CA allows all associated keys.");
            } else {
                match &credential.attestation.metadata {
                    AttestationMetadata::Packed { aaguid }
                    | AttestationMetadata::Tpm { aaguid, .. } => {
                        // If not present, fail.
                        if !att_ca_crt.aaguids().contains_key(aaguid) {
                            return Err(WebauthnError::AttestationUntrustedAaguid);
                        }
                    }
                    _ => {
                        // Fail
                        return Err(WebauthnError::AttestationFormatMissingAaguid);
                    }
                }
            }
        };

        // Verify that the credential public key alg is one of the allowed algorithms.
        let alg_valid = credential_algorithms
            .iter()
            .any(|alg| alg == &credential.cred.type_);

        if !alg_valid {
            error!(
                "Authenticator ignored requested algorithm set - {:?} - {:?}",
                credential.cred.type_, credential_algorithms
            );
            return Err(WebauthnError::CredentialAlteredAlgFromRequest);
        }

        // OUT OF SPEC - Allow rejection of synchronised credentials if desired by the caller.
        if !allow_synchronised_authenticators && credential.backup_eligible {
            error!("Credential counter is 0 - may indicate that it is a passkey and not bound to hardware.");
            return Err(WebauthnError::CredentialMayNotBeHardwareBound);
        }

        // OUT OF SPEC - It is invalid for a credential to indicate it is backed up
        // but not that it is elligible for backup
        if credential.backup_state && !credential.backup_eligible {
            error!("Credential indicates it is backed up, but has not declared valid backup elligibility");
            return Err(WebauthnError::CredentialMayNotBeHardwareBound);
        }

        // OUT OF SPEC - exclude any credential that is in our exclude list.
        let excluded = exclude_credentials
            .iter()
            .any(|credid| credid.as_slice() == credential.cred_id.as_slice());

        if excluded {
            return Err(WebauthnError::CredentialAlteredAlgFromRequest);
        }

        // If the attestation statement attStmt verified successfully and is found to be trustworthy,
        // then register the new credential with the account that was denoted in options.user:
        //
        // For us, we return the credential for the caller to persist.
        // If trust failed, we have already returned an Err before this point.

        // Associate the credentialId with the transport hints returned by calling
        // credential.response.getTransports(). This value SHOULD NOT be modified before or after
        // storing it. It is RECOMMENDED to use this value to populate the transports of the
        // allowCredentials option in future get() calls to help the client know how to find a
        // suitable authenticator.
        //
        // Done as part of credential construction if the transports are valid/trusted.

        Ok(credential)
    }

    // https://www.w3.org/TR/webauthn-3/#sctn-verifying-assertion
    pub(crate) fn verify_credential_internal(
        &self,
        rsp: &PublicKeyCredential,
        policy: UserVerificationPolicy,
        chal: &ChallengeRef,
        cred: &Credential,
        appid: &Option<String>,
        allow_backup_eligible_upgrade: bool,
    ) -> Result<AuthenticatorData<Authentication>, WebauthnError> {
        // Steps 1 through 7 are performed by the caller of this fn.

        // Let cData, authData and sig denote the value of credential’s response's clientDataJSON,
        // authenticatorData, and signature respectively.
        //
        // Let JSONtext be the result of running UTF-8 decode on the value of cData.
        let data = AuthenticatorAssertionResponse::try_from(&rsp.response).map_err(|e| {
            debug!("AuthenticatorAssertionResponse::try_from -> {:?}", e);
            e
        })?;

        let c = &data.client_data;

        /*
        Let C, the client data claimed as used for the signature, be the result of running an
        implementation-specific JSON parser on JSONtext.
            Note: C may be any implementation-specific data structure representation, as long as
            C’s components are referenceable, as required by this algorithm.
        */

        // Verify that the value of C.type is the string webauthn.get.
        if c.type_ != "webauthn.get" {
            return Err(WebauthnError::InvalidClientDataType);
        }

        // Verify that the value of C.challenge matches the challenge that was sent to the
        // authenticator in the PublicKeyCredentialRequestOptions passed to the get() call.
        if c.challenge.as_slice() != chal.as_ref() {
            return Err(WebauthnError::MismatchedChallenge);
        }

        // Verify that the value of C.origin matches one of our allowed origins.
        if !self.allowed_origins.iter().any(|origin| {
            Self::origins_match(
                self.allow_subdomains_origin,
                self.allow_any_port,
                &c.origin,
                origin,
            )
        }) {
            return Err(WebauthnError::InvalidRPOrigin);
        }

        // Verify that the value of C.tokenBinding.status matches the state of Token Binding for the
        // TLS connection over which the attestation was obtained. If Token Binding was used on that
        // TLS connection, also verify that C.tokenBinding.id matches the base64url encoding of the
        // Token Binding ID for the connection.

        // Verify that the rpIdHash in authData is the SHA-256 hash of the RP ID expected by the Relying Party.
        // Note that if we have an appid stored in the state and the client indicates it has used the appid extension,
        // we also check the hash against this appid in addition to the Relying Party
        let has_appid_enabled = rsp.extensions.appid.unwrap_or(false);

        let appid_hash = if has_appid_enabled {
            appid.as_ref().map(|id| compute_sha256(id.as_bytes()))
        } else {
            None
        };

        if !(data.authenticator_data.rp_id_hash == self.rp_id_hash
            || Some(&data.authenticator_data.rp_id_hash) == appid_hash.as_ref())
        {
            return Err(WebauthnError::InvalidRPIDHash);
        }

        // Verify that the User Present bit of the flags in authData is set.
        if !data.authenticator_data.user_present {
            return Err(WebauthnError::UserNotPresent);
        }

        // If user verification is required for this assertion, verify that the User Verified bit of
        // the flags in authData is set.
        //
        // We also check the historical policy too. See designs/authentication-use-cases.md

        match (&policy, &cred.registration_policy) {
            (_, UserVerificationPolicy::Required) | (UserVerificationPolicy::Required, _) => {
                // If we requested required at registration or now, enforce that.
                if !data.authenticator_data.user_verified {
                    return Err(WebauthnError::UserNotVerified);
                }
            }
            (_, UserVerificationPolicy::Preferred) => {
                // If we asked for Preferred at registration, we MAY have established to the user
                // that they are required to enter a pin, so we SHOULD enforce this.
                if cred.user_verified && !data.authenticator_data.user_verified {
                    debug!("Token registered UV=preferred, enforcing UV policy.");
                    return Err(WebauthnError::UserNotVerified);
                }
            }
            // Pass - we can not know if verification was requested to the client in the past correctly.
            // This means we can't know what it's behaviour is at the moment.
            // We must allow unverified tokens now.
            _ => {}
        }

        // OUT OF SPEC - if the backup elligibility of this device has changed, this may represent
        // a compromise of the credential, tampering with the device, or some other change to its
        // risk profile from when it was originally enrolled. Reject the authentication if this
        // situation occurs.

        if cred.backup_eligible != data.authenticator_data.backup_eligible {
            if allow_backup_eligible_upgrade
                && !cred.backup_eligible
                && data.authenticator_data.backup_eligible
            {
                debug!("Credential backup elligibility has changed!");
            } else {
                error!("Credential backup elligibility has changed!");
                return Err(WebauthnError::CredentialBackupElligibilityInconsistent);
            }
        }

        // OUT OF SPEC - It is invalid for a credential to indicate it is backed up
        // but not that it is elligible for backup
        if data.authenticator_data.backup_state && !cred.backup_eligible {
            error!("Credential indicates it is backed up, but has not declared valid backup elligibility");
            return Err(WebauthnError::CredentialMayNotBeHardwareBound);
        }

        // Verify that the values of the client extension outputs in clientExtensionResults and the
        // authenticator extension outputs in the extensions in authData are as expected, considering
        // the client extension input values that were given as the extensions option in the get()
        // call. In particular, any extension identifier values in the clientExtensionResults and
        // the extensions in authData MUST be also be present as extension identifier values in the
        // extensions member of options, i.e., no extensions are present that were not requested. In
        // the general case, the meaning of "are as expected" is specific to the Relying Party and
        // which extensions are in use.
        //
        // Note: Since all extensions are OPTIONAL for both the client and the authenticator, the
        // Relying Party MUST be prepared to handle cases where none or not all of the requested
        // extensions were acted upon.
        debug!("extensions: {:?}", data.authenticator_data.extensions);

        // Let hash be the result of computing a hash over the cData using SHA-256.
        let client_data_json_hash = compute_sha256(data.client_data_bytes.as_slice());

        // Using the credential public key, verify that sig is a valid signature
        // over the binary concatenation of authData and hash.
        // Note: This verification step is compatible with signatures generated by FIDO U2F
        // authenticators. See §6.1.2 FIDO U2F Signature Format Compatibility.

        let verification_data: Vec<u8> = data
            .authenticator_data_bytes
            .iter()
            .chain(client_data_json_hash.iter())
            .copied()
            .collect();

        let verified = cred
            .cred
            .verify_signature(&data.signature, &verification_data)?;

        if !verified {
            return Err(WebauthnError::AuthenticationFailure);
        }

        Ok(data.authenticator_data)
    }

    /// Generate a new challenge builder for client authentication. This is the first
    /// step in authentication of a credential. This function will return an
    /// authentication builder allowing you to customise the parameters that will be
    /// sent to the client.
    ///
    /// If creds is an empty `Vec` this implies a discoverable authentication attempt.
    pub fn new_challenge_authenticate_builder(
        &self,
        creds: Vec<Credential>,
        policy: Option<UserVerificationPolicy>,
    ) -> Result<ChallengeAuthenticateBuilder, WebauthnError> {
        let policy = if let Some(policy) = policy {
            policy
        } else {
            let policy = creds
                .first()
                .map(|cred| cred.registration_policy.to_owned())
                .ok_or(WebauthnError::CredentialNotFound)?;

            for cred in creds.iter() {
                if cred.registration_policy != policy {
                    return Err(WebauthnError::InconsistentUserVerificationPolicy);
                }
            }

            policy
        };

        Ok(ChallengeAuthenticateBuilder {
            creds,
            policy,
            extensions: Default::default(),
            allow_backup_eligible_upgrade: Default::default(),
            hints: Default::default(),
        })
    }

    /// Generate a new challenge for client authentication from the parameters defined by the
    /// [ChallengeAuthenticateBuilder].
    ///
    /// This function will return:
    ///
    /// * a [RequestChallengeResponse], which is sent to the client (and can be serialised as JSON).
    ///   A web application would then pass the structure to the browser's navigator.credentials.create() API to trigger authentication.
    ///
    /// * an [AuthenticationState], which must be persisted on the server side. Your application
    ///   must associate the state with a private session ID, to prevent use in other sessions.
    pub fn generate_challenge_authenticate(
        &self,
        challenge_builder: ChallengeAuthenticateBuilder,
    ) -> Result<(RequestChallengeResponse, AuthenticationState), WebauthnError> {
        let ChallengeAuthenticateBuilder {
            creds,
            policy,
            extensions,
            allow_backup_eligible_upgrade,
            hints,
        } = challenge_builder;

        let chal = self.generate_challenge();

        // Get the user's existing creds if any.
        let ac = creds
            .iter()
            .map(|cred| AllowCredentials {
                type_: "public-key".to_string(),
                id: cred.cred_id.as_ref().into(),
                transports: cred.transports.clone(),
            })
            .collect();

        // Extract the appid from the extensions to store it in the AuthenticationState
        let appid = extensions.as_ref().and_then(|e| e.appid.clone());

        let timeout_millis =
            u32::try_from(self.authenticator_timeout.as_millis()).expect("Timeout too large");

        // Store the chal associated to the user.
        // Now put that into the correct challenge format
        let r = RequestChallengeResponse {
            public_key: PublicKeyCredentialRequestOptions {
                challenge: chal.clone().into(),
                timeout: Some(timeout_millis),
                rp_id: self.rp_id.clone(),
                allow_credentials: ac,
                user_verification: policy,
                extensions,
                hints,
            },
            mediation: None,
        };
        let st = AuthenticationState {
            credentials: creds,
            policy,
            challenge: chal.into(),
            appid,
            allow_backup_eligible_upgrade,
        };
        Ok((r, st))
    }

    /// Process an authenticate response from the authenticator and browser. This
    /// is the output of `navigator.credentials.get()`, which is processed by this
    /// function. If the authentication fails, appropriate errors will be returned.
    ///
    /// This requires the associated AuthenticationState that was created by
    /// generate_challenge_authenticate
    ///
    /// On successful authentication, an Ok result is returned. The Ok may contain the CredentialID
    /// and associated counter, which you *should* update for security purposes. If the Ok returns
    /// `None` then the credential does not have a counter.
    pub fn authenticate_credential(
        &self,
        rsp: &PublicKeyCredential,
        state: &AuthenticationState,
    ) -> Result<AuthenticationResult, WebauthnError> {
        // Steps 1 through 4 are client side.

        // https://w3c.github.io/webauthn/#verifying-assertion
        // Lookup challenge

        let AuthenticationState {
            credentials: creds,
            policy,
            challenge: chal,
            appid,
            allow_backup_eligible_upgrade,
        } = state;
        let chal: &ChallengeRef = chal.into();

        // If the allowCredentials option was given when this authentication ceremony was initiated,
        // verify that credential.id identifies one of the public key credentials that were listed in allowCredentials.
        //
        // We always supply allowCredentials in this library, so we expect creds as a vec of credentials
        // that would be equivalent to what was allowed.
        // trace!("rsp: {:?}", rsp);

        let cred = {
            // Identify the user being authenticated and verify that this user is the owner of the public
            // key credential source credentialSource identified by credential.id:
            //
            //  If the user was identified before the authentication ceremony was initiated, e.g., via a
            //  username or cookie,
            //      verify that the identified user is the owner of credentialSource. If
            //      credential.response.userHandle is present, let userHandle be its value. Verify that
            //      userHandle also maps to the same user.

            //  If the user was not identified before the authentication ceremony was initiated,
            //      verify that credential.response.userHandle is present, and that the user identified
            //      by this value is the owner of credentialSource.
            //
            //      Note: User-less mode is handled by calling `AuthenticationState::set_allowed_credentials`
            //      after the caller extracts the userHandle and verifies the credential Source

            // Using credential’s id attribute (or the corresponding rawId, if base64url encoding is
            // inappropriate for your use case), look up the corresponding credential public key.
            let mut found_cred: Option<&Credential> = None;
            for cred in creds {
                if cred.cred_id.as_slice() == rsp.raw_id.as_slice() {
                    found_cred = Some(cred);
                    break;
                }
            }

            found_cred.ok_or(WebauthnError::CredentialNotFound)?
        };

        // Identify the user being authenticated and verify that this user is the owner of the public
        // key credential source credentialSource identified by credential.id:

        //  - If the user was identified before the authentication ceremony was initiated, e.g.,
        //  via a username or cookie,
        //      verify that the identified user is the owner of credentialSource. If
        //      response.userHandle is present, let userHandle be its value. Verify that
        //      userHandle also maps to the same user.

        // - If the user was not identified before the authentication ceremony was initiated,
        //      verify that response.userHandle is present, and that the user identified by this
        //      value is the owner of credentialSource.

        // Using credential.id (or credential.rawId, if base64url encoding is inappropriate for your
        // use case), look up the corresponding credential public key and let credentialPublicKey be
        // that credential public key.

        // * Due to the design of this library, in the majority of workflows the user MUST be known
        // before we begin, else we would not have the allowed Credentials list. When we proceed to
        // allowing resident keys (client side discoverable) such as username-less, then we will need
        // to consider how to proceed here. For now, username-less is such a hot-mess due to RK handling
        // being basicly non-existant, that there is no point. As a result, we have already enforced
        // these conditions.

        let auth_data = self.verify_credential_internal(
            rsp,
            *policy,
            chal,
            cred,
            appid,
            *allow_backup_eligible_upgrade,
        )?;
        let mut needs_update = false;
        let counter = auth_data.counter;
        let user_verified = auth_data.user_verified;
        let backup_state = auth_data.backup_state;
        let backup_eligible = auth_data.backup_eligible;

        let extensions = process_authentication_extensions(&auth_data.extensions);

        if backup_state != cred.backup_state {
            needs_update = true;
        }

        if backup_eligible != cred.backup_eligible {
            needs_update = true;
        }

        // If the signature counter value authData.signCount is nonzero or the value stored in
        // conjunction with credential’s id attribute is nonzero, then run the following sub-step:
        if counter > 0 || cred.counter > 0 {
            // greater than the signature counter value stored in conjunction with credential’s id attribute.
            //       Update the stored signature counter value, associated with credential’s id attribute,
            //       to be the value of authData.signCount.
            // less than or equal to the signature counter value stored in conjunction with credential’s id attribute.
            //      This is a signal that the authenticator may be cloned, i.e. at least two copies
            //      of the credential private key may exist and are being used in parallel. Relying
            //      Parties should incorporate this information into their risk scoring. Whether the
            //      Relying Party updates the stored signature counter value in this case, or not,
            //      or fails the authentication ceremony or not, is Relying Party-specific.
            let counter_shows_compromise = auth_data.counter <= cred.counter;

            if counter > cred.counter {
                needs_update = true;
            }

            if self.require_valid_counter_value && counter_shows_compromise {
                return Err(WebauthnError::CredentialPossibleCompromise);
            }
        }

        Ok(AuthenticationResult {
            cred_id: cred.cred_id.clone(),
            needs_update,
            user_verified,
            backup_eligible,
            backup_state,
            counter,
            extensions,
        })
    }

    fn origins_match(
        allow_subdomains_origin: bool,
        allow_any_port: bool,
        ccd_url: &url::Url,
        cnf_url: &url::Url,
    ) -> bool {
        if ccd_url == cnf_url {
            return true;
        }
        if allow_subdomains_origin {
            match (ccd_url.origin(), cnf_url.origin()) {
                (
                    url::Origin::Tuple(ccd_scheme, ccd_host, ccd_port),
                    url::Origin::Tuple(cnf_scheme, cnf_host, cnf_port),
                ) => {
                    if ccd_scheme != cnf_scheme {
                        debug!("{} != {}", ccd_url, cnf_url);
                        return false;
                    }

                    if !allow_any_port && ccd_port != cnf_port {
                        debug!("{} != {}", ccd_url, cnf_url);
                        return false;
                    }

                    let valid = match (ccd_host, cnf_host) {
                        (url::Host::Domain(ccd_domain), url::Host::Domain(cnf_domain)) => {
                            ccd_domain.ends_with(&cnf_domain)
                        }
                        (a, b) => a == b,
                    };

                    if valid {
                        true
                    } else {
                        debug!("Domain/IP in origin do not match");
                        false
                    }
                }
                _ => {
                    debug!("Origin is opaque");
                    false
                }
            }
        } else if ccd_url.origin() != cnf_url.origin() || !ccd_url.origin().is_tuple() {
            if ccd_url.host() == cnf_url.host()
                && ccd_url.scheme() == cnf_url.scheme()
                && allow_any_port
            {
                true
            } else {
                debug!("{} != {}", ccd_url, cnf_url);
                false
            }
        } else {
            true
        }
    }

    /// Returns the RP name
    pub fn rp_name(&self) -> &str {
        self.rp_name.as_str()
    }
}

#[cfg(test)]
mod tests {
    #![allow(clippy::panic)]

    use crate::constants::CHALLENGE_SIZE_BYTES;
    use crate::core::{CreationChallengeResponse, RegistrationState, WebauthnError};
    use crate::internals::*;
    use crate::proto::*;
    use crate::WebauthnCore as Webauthn;
    use base64::{engine::general_purpose::STANDARD, Engine};
    use base64urlsafedata::{Base64UrlSafeData, HumanBinaryData};
    use std::time::Duration;
    use url::Url;

    use webauthn_rs_device_catalog::data::{
        android::ANDROID_SOFTWARE_ROOT_CA, apple::APPLE_WEBAUTHN_ROOT_CA_PEM,
        google::GOOGLE_SAFETYNET_CA_OLD,
        microsoft::MICROSOFT_TPM_ROOT_CERTIFICATE_AUTHORITY_2014_PEM,
        yubico::YUBICO_U2F_ROOT_CA_SERIAL_457200631_PEM,
    };

    const AUTHENTICATOR_TIMEOUT: Duration = Duration::from_secs(60);

    // Test the crypto operations of the webauthn impl

    #[test]
    fn test_registration_yk() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "http://127.0.0.1:8080/auth",
            "127.0.0.1",
            vec![Url::parse("http://127.0.0.1:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );
        // Generated by a yubico 5
        // Make a "fake" challenge, where we know what the values should be ....

        let zero_chal = Challenge::new((0..CHALLENGE_SIZE_BYTES).map(|_| 0).collect::<Vec<u8>>());

        // This is the json challenge this would generate in this case, with the rp etc.
        // {"publicKey":{"rp":{"name":"http://127.0.0.1:8080/auth"},"user":{"id":"xxx","name":"xxx","displayName":"xxx"},"challenge":"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=","pubKeyCredParams":[{"type":"public-key","alg":-7}],"timeout":6000,"attestation":"direct"}}

        // And this is the response, from a real device. Let's register it!

        let rsp = r#"
        {
            "id":"0xYE4bQ_HZM51-XYwp7WHJu8RfeA2Oz3_9HnNIZAKqRTz9gsUlF3QO7EqcJ0pgLSwDcq6cL1_aQpTtKLeGu6Ig",
            "rawId":"0xYE4bQ_HZM51-XYwp7WHJu8RfeA2Oz3_9HnNIZAKqRTz9gsUlF3QO7EqcJ0pgLSwDcq6cL1_aQpTtKLeGu6Ig",
            "response":{
                 "attestationObject":"o2NmbXRoZmlkby11MmZnYXR0U3RtdKJjc2lnWEcwRQIhALjRb43YFcbJ3V9WiYPpIrZkhgzAM6KTR8KIjwCXejBCAiAO5Lvp1VW4dYBhBDv7HZIrxZb1SwKKYOLfFRXykRxMqGN4NWOBWQLBMIICvTCCAaWgAwIBAgIEGKxGwDANBgkqhkiG9w0BAQsFADAuMSwwKgYDVQQDEyNZdWJpY28gVTJGIFJvb3QgQ0EgU2VyaWFsIDQ1NzIwMDYzMTAgFw0xNDA4MDEwMDAwMDBaGA8yMDUwMDkwNDAwMDAwMFowbjELMAkGA1UEBhMCU0UxEjAQBgNVBAoMCVl1YmljbyBBQjEiMCAGA1UECwwZQXV0aGVudGljYXRvciBBdHRlc3RhdGlvbjEnMCUGA1UEAwweWXViaWNvIFUyRiBFRSBTZXJpYWwgNDEzOTQzNDg4MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEeeo7LHxJcBBiIwzSP-tg5SkxcdSD8QC-hZ1rD4OXAwG1Rs3Ubs_K4-PzD4Hp7WK9Jo1MHr03s7y-kqjCrutOOqNsMGowIgYJKwYBBAGCxAoCBBUxLjMuNi4xLjQuMS40MTQ4Mi4xLjcwEwYLKwYBBAGC5RwCAQEEBAMCBSAwIQYLKwYBBAGC5RwBAQQEEgQQy2lIHo_3QDmT7AonKaFUqDAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4IBAQCXnQOX2GD4LuFdMRx5brr7Ivqn4ITZurTGG7tX8-a0wYpIN7hcPE7b5IND9Nal2bHO2orh_tSRKSFzBY5e4cvda9rAdVfGoOjTaCW6FZ5_ta2M2vgEhoz5Do8fiuoXwBa1XCp61JfIlPtx11PXm5pIS2w3bXI7mY0uHUMGvxAzta74zKXLslaLaSQibSKjWKt9h-SsXy4JGqcVefOlaQlJfXL1Tga6wcO0QTu6Xq-Uw7ZPNPnrpBrLauKDd202RlN4SP7ohL3d9bG6V5hUz_3OusNEBZUn5W3VmPj1ZnFavkMB3RkRMOa58MZAORJT4imAPzrvJ0vtv94_y71C6tZ5aGF1dGhEYXRhWMQSyhe0mvIolDbzA-AWYDCiHlJdJm4gkmdDOAGo_UBxoEEAAAAAAAAAAAAAAAAAAAAAAAAAAABA0xYE4bQ_HZM51-XYwp7WHJu8RfeA2Oz3_9HnNIZAKqRTz9gsUlF3QO7EqcJ0pgLSwDcq6cL1_aQpTtKLeGu6IqUBAgMmIAEhWCCe1KvqpcVWN416_QZc8vJynt3uo3_WeJ2R4uj6kJbaiiJYIDC5ssxxummKviGgLoP9ZLFb836A9XfRO7op18QY3i5m",
                 "clientDataJSON":"eyJjaGFsbGVuZ2UiOiJBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cDovLzEyNy4wLjAuMTo4MDgwIiwidHlwZSI6IndlYmF1dGhuLmNyZWF0ZSJ9"
            },
            "type":"public-key"}
        "#;
        // turn it into our "deserialised struct"
        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(rsp).unwrap();

        // Now register, providing our fake challenge.
        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &zero_chal,
            &[],
            &[COSEAlgorithm::ES256],
            Some(&YUBICO_U2F_ROOT_CA_SERIAL_457200631_PEM.try_into().unwrap()),
            false,
            &RequestRegistrationExtensions::default(),
            true,
        );
        trace!("{:?}", result);
        assert!(result.is_ok());
    }

    // These are vectors from https://github.com/duo-labs/webauthn
    #[test]
    fn test_registration_duo_go() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.io",
            "webauthn.io",
            vec![Url::parse("https://webauthn.io").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(
            STANDARD
                .decode("+Ri5NZTzJ8b6mvW3TVScLotEoALfgBa2Bn4YSaIObHc=")
                .unwrap(),
        );

        let rsp = r#"
        {
                "id": "FOxcmsqPLNCHtyILvbNkrtHMdKAeqSJXYZDbeFd0kc5Enm8Kl6a0Jp0szgLilDw1S4CjZhe9Z2611EUGbjyEmg",
                "rawId": "FOxcmsqPLNCHtyILvbNkrtHMdKAeqSJXYZDbeFd0kc5Enm8Kl6a0Jp0szgLilDw1S4CjZhe9Z2611EUGbjyEmg",
                "response": {
                        "attestationObject": "o2NmbXRoZmlkby11MmZnYXR0U3RtdKJjc2lnWEYwRAIgfyIhwZj-fkEVyT1GOK8chDHJR2chXBLSRg6bTCjODmwCIHH6GXI_BQrcR-GHg5JfazKVQdezp6_QWIFfT4ltTCO2Y3g1Y4FZAlMwggJPMIIBN6ADAgECAgQSNtF_MA0GCSqGSIb3DQEBCwUAMC4xLDAqBgNVBAMTI1l1YmljbyBVMkYgUm9vdCBDQSBTZXJpYWwgNDU3MjAwNjMxMCAXDTE0MDgwMTAwMDAwMFoYDzIwNTAwOTA0MDAwMDAwWjAxMS8wLQYDVQQDDCZZdWJpY28gVTJGIEVFIFNlcmlhbCAyMzkyNTczNDEwMzI0MTA4NzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABNNlqR5emeDVtDnA2a-7h_QFjkfdErFE7bFNKzP401wVE-QNefD5maviNnGVk4HJ3CsHhYuCrGNHYgTM9zTWriGjOzA5MCIGCSsGAQQBgsQKAgQVMS4zLjYuMS40LjEuNDE0ODIuMS41MBMGCysGAQQBguUcAgEBBAQDAgUgMA0GCSqGSIb3DQEBCwUAA4IBAQAiG5uzsnIk8T6-oyLwNR6vRklmo29yaYV8jiP55QW1UnXdTkEiPn8mEQkUac-Sn6UmPmzHdoGySG2q9B-xz6voVQjxP2dQ9sgbKd5gG15yCLv6ZHblZKkdfWSrUkrQTrtaziGLFSbxcfh83vUjmOhDLFC5vxV4GXq2674yq9F2kzg4nCS4yXrO4_G8YWR2yvQvE2ffKSjQJlXGO5080Ktptplv5XN4i5lS-AKrT5QRVbEJ3B4g7G0lQhdYV-6r4ZtHil8mF4YNMZ0-RaYPxAaYNWkFYdzOZCaIdQbXRZefgGfbMUiAC2gwWN7fiPHV9eu82NYypGU32OijG9BjhGt_aGF1dGhEYXRhWMR0puqSE8mcL3SyJJKzIM9AJiqUwalQoDl_KSULYIQe8EEAAAAAAAAAAAAAAAAAAAAAAAAAAABAFOxcmsqPLNCHtyILvbNkrtHMdKAeqSJXYZDbeFd0kc5Enm8Kl6a0Jp0szgLilDw1S4CjZhe9Z2611EUGbjyEmqUBAgMmIAEhWCD_ap3Q9zU8OsGe967t48vyRxqn8NfFTk307mC1WsH2ISJYIIcqAuW3MxhU0uDtaSX8-Ftf_zeNJLdCOEjZJGHsrLxH",
                        "clientDataJSON": "eyJjaGFsbGVuZ2UiOiItUmk1TlpUeko4YjZtdlczVFZTY0xvdEVvQUxmZ0JhMkJuNFlTYUlPYkhjIiwib3JpZ2luIjoiaHR0cHM6Ly93ZWJhdXRobi5pbyIsInR5cGUiOiJ3ZWJhdXRobi5jcmVhdGUifQ"
                },
                "type": "public-key"
        }
        "#;
        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(rsp).unwrap();
        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            chal.as_ref(),
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            true,
        );
        trace!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_registration_packed_attestation() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "localhost:8443/auth",
            "localhost",
            vec![Url::parse("https://localhost:8443").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(
            STANDARD
                .decode("lP6mWNAtG+/Vv15iM7lb/XRkdWMvVQ+lTyKwZuOg1Vo=")
                .unwrap(),
        );

        // Example generated using navigator.credentials.create on Chrome Version 77.0.3865.120
        // using Touch ID on MacBook running MacOS 10.15
        let rsp = r#"{
                        "id":"ATk_7QKbi_ntSdp16LXeU6RDf9YnRLIDTCqEjJFzc6rKBhbqoSYccxNa",
                        "rawId":"ATk_7QKbi_ntSdp16LXeU6RDf9YnRLIDTCqEjJFzc6rKBhbqoSYccxNa",
                        "response":{
                            "attestationObject":"o2NmbXRmcGFja2VkZ2F0dFN0bXSiY2FsZyZjc2lnWEcwRQIgLXPjBtVEhBH3KdUDFFk3LAd9EtHogllIf48vjX4wgfECIQCXOymmfg12FPMXEdwpSjjtmrvki4K8y0uYxqWN5Bw6DGhhdXRoRGF0YViuSZYN5YgOjGh0NBcPZHZgW4_krrmihjLHmVzzuoMdl2NFXaqejq3OAAI1vMYKZIsLJfHwVQMAKgE5P-0Cm4v57Unadei13lOkQ3_WJ0SyA0wqhIyRc3OqygYW6qEmHHMTWqUBAgMmIAEhWCDNRS_Gw52ow5PNrC9OdFTFNudDmZO6Y3wmM9N8e0tJICJYIC09iIH5_RrT5tbS0PIw3srdAxYDMGao7yWgu0JFIEzT",
                            "clientDataJSON":"eyJjaGFsbGVuZ2UiOiJsUDZtV05BdEctX1Z2MTVpTTdsYl9YUmtkV012VlEtbFR5S3dadU9nMVZvIiwiZXh0cmFfa2V5c19tYXlfYmVfYWRkZWRfaGVyZSI6ImRvIG5vdCBjb21wYXJlIGNsaWVudERhdGFKU09OIGFnYWluc3QgYSB0ZW1wbGF0ZS4gU2VlIGh0dHBzOi8vZ29vLmdsL3lhYlBleCIsIm9yaWdpbiI6Imh0dHBzOi8vbG9jYWxob3N0Ojg0NDMiLCJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIn0"
                            },
                        "type":"public-key"
                      }
        "#;
        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(rsp).unwrap();
        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        assert!(result.is_ok());
    }

    #[test]
    fn test_registration_packed_attestaion_fails_with_bad_cred_protect() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "localhost:8080/auth",
            "localhost",
            vec![Url::parse("http://localhost:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(vec![
            125, 119, 194, 67, 227, 22, 152, 134, 220, 143, 75, 119, 197, 165, 115, 149, 187, 153,
            211, 51, 215, 128, 225, 56, 110, 80, 52, 235, 149, 146, 101, 202,
        ]);

        let rsp = r#"{
            "id":"9KJylaUgVoWF2cF2qX5an7ZtPBFeRMXy-jMSGgNWCogxiyctVFtIcDKmkVmfKOgllffKJMyl4gFeDm8KaltrDw",
            "rawId":"9KJylaUgVoWF2cF2qX5an7ZtPBFeRMXy-jMSGgNWCogxiyctVFtIcDKmkVmfKOgllffKJMyl4gFeDm8KaltrDw",
            "response":{
                "attestationObject":"o2NmbXRmcGFja2VkZ2F0dFN0bXSjY2FsZyZjc2lnWEYwRAIgZEq9euYGkqTP4VMBs-5fruhwAPSyKjOlr2THNZGvZ3gCIHww2gAgZXvZcIwcSiUF3fHhaNL0uj8V5rOLHyGRJz81Y3g1Y4FZAsEwggK9MIIBpaADAgECAgQej4c0MA0GCSqGSIb3DQEBCwUAMC4xLDAqBgNVBAMTI1l1YmljbyBVMkYgUm9vdCBDQSBTZXJpYWwgNDU3MjAwNjMxMCAXDTE0MDgwMTAwMDAwMFoYDzIwNTAwOTA0MDAwMDAwWjBuMQswCQYDVQQGEwJTRTESMBAGA1UECgwJWXViaWNvIEFCMSIwIAYDVQQLDBlBdXRoZW50aWNhdG9yIEF0dGVzdGF0aW9uMScwJQYDVQQDDB5ZdWJpY28gVTJGIEVFIFNlcmlhbCA1MTI3MjI3NDAwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASoefgjOO0UlLrAcEvMf8Zj0bJxcVl2JDEBx2BRFdfBUp4oHBxnMi04S1zVXdPpgY1f2FwirzJuDGT8IK_jPyNmo2wwajAiBgkrBgEEAYLECgIEFTEuMy42LjEuNC4xLjQxNDgyLjEuNzATBgsrBgEEAYLlHAIBAQQEAwIEMDAhBgsrBgEEAYLlHAEBBAQSBBAvwFefgRNH6rEWu1qNuSAqMAwGA1UdEwEB_wQCMAAwDQYJKoZIhvcNAQELBQADggEBAIaT_2LfDVd51HSNf8jRAicxio5YDmo6V8EI6U4Dw4Vos2aJT85WJL5KPv1_NBGLPZk3Q_eSoZiRYMj8muCwTj357hXj6IwE_IKo3L9YGOEI3MKWhXeuef9mK5RzTj3sRZcwXXPm5V7ivrnNlnjKCTXlM-tjj44m-ruBfNpEH76YMYMq5fbirZkvnrvbTGIji4-NerSB1tMmO82_nkpXVQNwmIrVgTRA-gMsrbZyPK3Y-Ne6gJ91tDz_oKW5rdFCMu-dnhSBJjgjPEykqHO5-KyY4yuhkWdgbhWQn83bSi3_va5GICSfmmZGrIHkgy0RGf6_qnMaiC2iWneCfUbRkBdoYXV0aERhdGFY0kmWDeWIDoxodDQXD2R2YFuP5K65ooYyx5lc87qDHZdjxQAAAAEvwFefgRNH6rEWu1qNuSAqAED0onKVpSBWhYXZwXapflqftm08EV5ExfL6MxIaA1YKiDGLJy1UW0hwMqaRWZ8o6CWV98okzKXiAV4ObwpqW2sPpQECAyYgASFYIB_nQH-kBm4OmDfqezjFDr_t0Psz6JrylkEPWHFs2UB-Ilgg7xkwKc-IHHIwPI8EJ5ycM1zvWDnm4bCarn1LAWAU3Dqha2NyZWRQcm90ZWN0Aw",
                "clientDataJSON":"eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiZlhmQ1EtTVdtSWJjajB0M3hhVnpsYnVaMHpQWGdPRTRibEEwNjVXU1pjbyIsIm9yaWdpbiI6Imh0dHA6Ly9sb2NhbGhvc3Q6ODA4MCIsImNyb3NzT3JpZ2luIjpmYWxzZSwib3RoZXJfa2V5c19jYW5fYmVfYWRkZWRfaGVyZSI6ImRvIG5vdCBjb21wYXJlIGNsaWVudERhdGFKU09OIGFnYWluc3QgYSB0ZW1wbGF0ZS4gU2VlIGh0dHBzOi8vZ29vLmdsL3lhYlBleCJ9"
            },
            "type":"public-key"
        }"#;
        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(rsp).unwrap();

        trace!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_registration_packed_attestaion_works_with_valid_fido_aaguid_extension() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au/compat_test").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"qabSCYW_PPKKBAW5_qEsPF3Q3prQeYBORfDMArsoKdg\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp = r#"{
            "id": "eKSmfhLUwwmJpuD2IKaTopbbWKFv-qZAE4LXa2FGmTtRpvioMpeFhI8RqdsOGlBoQxJehEQyWyu7ECwPkVL5Hg",
            "rawId": "eKSmfhLUwwmJpuD2IKaTopbbWKFv-qZAE4LXa2FGmTtRpvioMpeFhI8RqdsOGlBoQxJehEQyWyu7ECwPkVL5Hg",
            "response": {
            "attestationObject": "o2NmbXRmcGFja2VkZ2F0dFN0bXSjY2FsZyZjc2lnWEcwRQIgW2gYNWvUDgxl8LB7rflbuJw_zvJCT5ddfDZNROTy0JYCIQDxuy3JLSHDIrEFYqDifFA_ZHttNfRqJAPgH4hedttVIWN4NWOBWQLBMIICvTCCAaWgAwIBAgIEHo-HNDANBgkqhkiG9w0BAQsFADAuMSwwKgYDVQQDEyNZdWJpY28gVTJGIFJvb3QgQ0EgU2VyaWFsIDQ1NzIwMDYzMTAgFw0xNDA4MDEwMDAwMDBaGA8yMDUwMDkwNDAwMDAwMFowbjELMAkGA1UEBhMCU0UxEjAQBgNVBAoMCVl1YmljbyBBQjEiMCAGA1UECwwZQXV0aGVudGljYXRvciBBdHRlc3RhdGlvbjEnMCUGA1UEAwweWXViaWNvIFUyRiBFRSBTZXJpYWwgNTEyNzIyNzQwMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEqHn4IzjtFJS6wHBLzH_GY9GycXFZdiQxAcdgURXXwVKeKBwcZzItOEtc1V3T6YGNX9hcIq8ybgxk_CCv4z8jZqNsMGowIgYJKwYBBAGCxAoCBBUxLjMuNi4xLjQuMS40MTQ4Mi4xLjcwEwYLKwYBBAGC5RwCAQEEBAMCBDAwIQYLKwYBBAGC5RwBAQQEEgQQL8BXn4ETR-qxFrtajbkgKjAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4IBAQCGk_9i3w1XedR0jX_I0QInMYqOWA5qOlfBCOlOA8OFaLNmiU_OViS-Sj79fzQRiz2ZN0P3kqGYkWDI_JrgsE49-e4V4-iMBPyCqNy_WBjhCNzCloV3rnn_ZiuUc0497EWXMF1z5uVe4r65zZZ4ygk15TPrY4-OJvq7gXzaRB--mDGDKuX24q2ZL56720xiI4uPjXq0gdbTJjvNv55KV1UDcJiK1YE0QPoDLK22cjyt2PjXuoCfdbQ8_6Clua3RQjLvnZ4UgSY4IzxMpKhzufismOMroZFnYG4VkJ_N20ot_72uRiAkn5pmRqyB5IMtERn-v6pzGogtolp3gn1G0ZAXaGF1dGhEYXRhWMRqubvw35oW-R27M7uxMvr50Xx4LEgmxuxw7O5Y2X71KkUAAAACL8BXn4ETR-qxFrtajbkgKgBAeKSmfhLUwwmJpuD2IKaTopbbWKFv-qZAE4LXa2FGmTtRpvioMpeFhI8RqdsOGlBoQxJehEQyWyu7ECwPkVL5HqUBAgMmIAEhWCBT_WnxT3SKAIGfnEKUi7xtZmnlcZRV-63N21154_r-xyJYIGuwu6BK1zp6D6EQ94VOcK1DuFWr58xI_PbeP5F1Nfe6",
            "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJxYWJTQ1lXX1BQS0tCQVc1X3FFc1BGM1EzcHJRZVlCT1JmRE1BcnNvS2RnIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cHM6Ly93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIn0"
            },
            "type": "public-key"
        }"#;

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(rsp).unwrap();

        trace!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_registration_packed_attestaion_fails_with_invalid_fido_aaguid_extension() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au/compat_test").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"qabSCYW_PPKKBAW5_qEsPF3Q3prQeYBORfDMArsoKdg\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp = r#"{
            "id": "eKSmfhLUwwmJpuD2IKaTopbbWKFv-qZAE4LXa2FGmTtRpvioMpeFhI8RqdsOGlBoQxJehEQyWyu7ECwPkVL5Hg",
            "rawId": "eKSmfhLUwwmJpuD2IKaTopbbWKFv-qZAE4LXa2FGmTtRpvioMpeFhI8RqdsOGlBoQxJehEQyWyu7ECwPkVL5Hg",
            "response": {
            "attestationObject": "o2NmbXRmcGFja2VkZ2F0dFN0bXSjY2FsZyZjc2lnWEcwRQIgW2gYNWvUDgxl8LB7rflbuJw_zvJCT5ddfDZNROTy0JYCIQDxuy3JLSHDIrEFYqDifFA_ZHttNfRqJAPgH4hedttVIWN4NWOBWQLBMIICvTCCAaWgAwIBAgIEHo-HNDANBgkqhkiG9w0BAQsFADAuMSwwKgYDVQQDEyNZdWJpY28gVTJGIFJvb3QgQ0EgU2VyaWFsIDQ1NzIwMDYzMTAgFw0xNDA4MDEwMDAwMDBaGA8yMDUwMDkwNDAwMDAwMFowbjELMAkGA1UEBhMCU0UxEjAQBgNVBAoMCVl1YmljbyBBQjEiMCAGA1UECwwZQXV0aGVudGljYXRvciBBdHRlc3RhdGlvbjEnMCUGA1UEAwweWXViaWNvIFUyRiBFRSBTZXJpYWwgNTEyNzIyNzQwMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEqHn4IzjtFJS6wHBLzH_GY9GycXFZdiQxAcdgURXXwVKeKBwcZzItOEtc1V3T6YGNX9hcIq8ybgxk_CCv4z8jZqNsMGowIgYJKwYBBAGCxAoCBBUxLjMuNi4xLjQuMS40MTQ4Mi4xLjcwEwYLKwYBBAGC5RwCAQEEBAMCBDAwIQYLKwYBBAGC5RwBAQQEEgQQXXXXXXXXXXXXXXXXXXXXXjAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4IBAQCGk_9i3w1XedR0jX_I0QInMYqOWA5qOlfBCOlOA8OFaLNmiU_OViS-Sj79fzQRiz2ZN0P3kqGYkWDI_JrgsE49-e4V4-iMBPyCqNy_WBjhCNzCloV3rnn_ZiuUc0497EWXMF1z5uVe4r65zZZ4ygk15TPrY4-OJvq7gXzaRB--mDGDKuX24q2ZL56720xiI4uPjXq0gdbTJjvNv55KV1UDcJiK1YE0QPoDLK22cjyt2PjXuoCfdbQ8_6Clua3RQjLvnZ4UgSY4IzxMpKhzufismOMroZFnYG4VkJ_N20ot_72uRiAkn5pmRqyB5IMtERn-v6pzGogtolp3gn1G0ZAXaGF1dGhEYXRhWMRqubvw35oW-R27M7uxMvr50Xx4LEgmxuxw7O5Y2X71KkUAAAACL8BXn4ETR-qxFrtajbkgKgBAeKSmfhLUwwmJpuD2IKaTopbbWKFv-qZAE4LXa2FGmTtRpvioMpeFhI8RqdsOGlBoQxJehEQyWyu7ECwPkVL5HqUBAgMmIAEhWCBT_WnxT3SKAIGfnEKUi7xtZmnlcZRV-63N21154_r-xyJYIGuwu6BK1zp6D6EQ94VOcK1DuFWr58xI_PbeP5F1Nfe6",
            "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJxYWJTQ1lXX1BQS0tCQVc1X3FFc1BGM1EzcHJRZVlCT1JmRE1BcnNvS2RnIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cHM6Ly93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIn0"
            },
            "type": "public-key"
        }"#;

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(rsp).unwrap();

        trace!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::AttestationCertificateAAGUIDMismatch)
        ));
    }

    #[test]
    fn test_authentication() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "localhost:8080/auth",
            "localhost",
            vec![Url::parse("http://localhost:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        // Generated by a yubico 5
        // Make a "fake" challenge, where we know what the values should be ....

        let zero_chal = Challenge::new(vec![
            90, 5, 243, 254, 68, 239, 221, 101, 20, 214, 76, 60, 134, 111, 142, 26, 129, 146, 225,
            144, 135, 95, 253, 219, 18, 161, 199, 216, 251, 213, 167, 195,
        ]);

        // Create the fake credential that we know is associated
        let cred = Credential {
            cred_id: HumanBinaryData::from(vec![
                106, 223, 133, 124, 161, 172, 56, 141, 181, 18, 27, 66, 187, 181, 113, 251, 187,
                123, 20, 169, 41, 80, 236, 138, 92, 137, 4, 4, 16, 255, 188, 47, 158, 202, 111,
                192, 117, 110, 152, 245, 95, 22, 200, 172, 71, 154, 40, 181, 212, 64, 80, 17, 238,
                238, 21, 13, 27, 145, 140, 27, 208, 101, 166, 81,
            ]),
            cred: COSEKey {
                type_: COSEAlgorithm::ES256,
                key: COSEKeyType::EC_EC2(COSEEC2Key {
                    curve: ECDSACurve::SECP256R1,
                    x: [
                        46, 121, 76, 233, 118, 208, 250, 74, 227, 182, 8, 145, 45, 46, 5, 9, 199,
                        186, 84, 83, 7, 237, 130, 73, 16, 90, 17, 54, 33, 255, 54, 56,
                    ]
                    .to_vec()
                    .into(),
                    y: [
                        117, 105, 1, 23, 253, 223, 67, 135, 253, 219, 253, 223, 17, 247, 91, 197,
                        205, 225, 143, 59, 47, 138, 70, 120, 74, 155, 177, 177, 166, 233, 48, 71,
                    ]
                    .to_vec()
                    .into(),
                }),
            },
            counter: 1,
            transports: None,
            user_verified: false,
            backup_eligible: false,
            backup_state: false,
            registration_policy: UserVerificationPolicy::Discouraged_DO_NOT_USE,
            extensions: RegisteredExtensions::none(),
            attestation: ParsedAttestation {
                data: ParsedAttestationData::None,
                metadata: AttestationMetadata::None,
            },
            attestation_format: AttestationFormat::None,
        };

        // Persist it to our fake db.

        // Captured authentication attempt
        let rsp = r#"
        {
            "id":"at-FfKGsOI21EhtCu7Vx-7t7FKkpUOyKXIkEBBD_vC-eym_AdW6Y9V8WyKxHmii11EBQEe7uFQ0bkYwb0GWmUQ",
            "rawId":"at-FfKGsOI21EhtCu7Vx-7t7FKkpUOyKXIkEBBD_vC-eym_AdW6Y9V8WyKxHmii11EBQEe7uFQ0bkYwb0GWmUQ",
            "response":{
                "authenticatorData":"SZYN5YgOjGh0NBcPZHZgW4_krrmihjLHmVzzuoMdl2MBAAAAFA",
                "clientDataJSON":"eyJjaGFsbGVuZ2UiOiJXZ1h6X2tUdjNXVVUxa3c4aG0tT0dvR1M0WkNIWF8zYkVxSEgyUHZWcDhNIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cDovL2xvY2FsaG9zdDo4MDgwIiwidHlwZSI6IndlYmF1dGhuLmdldCJ9",
                "signature":"MEYCIQDmLVOqv85cdRup4Fr8Pf9zC4AWO-XKBJqa8xPwYFCCMAIhAOiExLoyes0xipmUmq0BVlqJaCKLn_MFKG9GIDsCGq_-",
                "userHandle":null
            },
            "type":"public-key"
        }
        "#;
        let rsp_d: PublicKeyCredential = serde_json::from_str(rsp).unwrap();

        // Now verify it!
        let r = wan.verify_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &zero_chal,
            &cred,
            &None,
            false,
        );
        trace!("RESULT: {:?}", r);
        assert!(r.is_ok());

        // Captured authentication attempt, this mentions the appid extension has been used, but we still provide a valid RPID
        let rsp = r#"
        {
            "id":"at-FfKGsOI21EhtCu7Vx-7t7FKkpUOyKXIkEBBD_vC-eym_AdW6Y9V8WyKxHmii11EBQEe7uFQ0bkYwb0GWmUQ",
            "rawId":"at-FfKGsOI21EhtCu7Vx-7t7FKkpUOyKXIkEBBD_vC-eym_AdW6Y9V8WyKxHmii11EBQEe7uFQ0bkYwb0GWmUQ",
            "extensions": {
                "appid": true
            },
            "response":{
                "authenticatorData":"SZYN5YgOjGh0NBcPZHZgW4_krrmihjLHmVzzuoMdl2MBAAAAFA",
                "clientDataJSON":"eyJjaGFsbGVuZ2UiOiJXZ1h6X2tUdjNXVVUxa3c4aG0tT0dvR1M0WkNIWF8zYkVxSEgyUHZWcDhNIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cDovL2xvY2FsaG9zdDo4MDgwIiwidHlwZSI6IndlYmF1dGhuLmdldCJ9",
                "signature":"MEYCIQDmLVOqv85cdRup4Fr8Pf9zC4AWO-XKBJqa8xPwYFCCMAIhAOiExLoyes0xipmUmq0BVlqJaCKLn_MFKG9GIDsCGq_-",
                "userHandle":null
            },
            "type":"public-key"
        }
        "#;
        let rsp_d: PublicKeyCredential = serde_json::from_str(rsp).unwrap();

        // Now verify it, as the RPID is valid, the appid should be ignored
        let r = wan.verify_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &zero_chal,
            &cred,
            &Some(String::from("https://unused.local")),
            false,
        );
        trace!("RESULT: {:?}", r);
        assert!(r.is_ok());
    }

    #[test]
    fn test_authentication_appid() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://testing.local",
            "testing.local",
            vec![Url::parse("https://testing.local").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        // Generated by a yubico 5
        // Make a "fake" challenge, where we know what the values should be ....

        let zero_chal = Challenge::new(vec![
            160, 127, 213, 174, 150, 36, 228, 190, 41, 61, 216, 14, 171, 191, 75, 203, 99, 59, 4,
            252, 49, 90, 235, 36, 220, 165, 159, 201, 58, 225, 248, 142,
        ]);

        // Create the fake credential that we know is associated
        let cred = Credential {
            counter: 1,
            transports: None,
            cred_id: HumanBinaryData::from(vec![
                179, 64, 237, 0, 28, 248, 197, 30, 213, 228, 250, 139, 28, 11, 156, 130, 69, 242,
                21, 48, 84, 77, 103, 163, 66, 204, 167, 147, 82, 214, 212,
            ]),
            cred: COSEKey {
                type_: COSEAlgorithm::ES256,
                key: COSEKeyType::EC_EC2(COSEEC2Key {
                    curve: ECDSACurve::SECP256R1,
                    x: [
                        187, 71, 18, 101, 166, 110, 166, 38, 116, 119, 74, 4, 183, 104, 24, 46,
                        245, 24, 227, 143, 161, 136, 37, 186, 140, 221, 228, 115, 81, 175, 50, 51,
                    ]
                    .to_vec()
                    .into(),
                    y: [
                        13, 59, 59, 158, 149, 197, 116, 228, 99, 12, 235, 185, 190, 110, 251, 154,
                        226, 143, 75, 26, 44, 136, 244, 245, 243, 4, 40, 223, 22, 253, 224, 95,
                    ]
                    .to_vec()
                    .into(),
                }),
            },
            user_verified: false,
            backup_eligible: false,
            backup_state: false,
            registration_policy: UserVerificationPolicy::Discouraged_DO_NOT_USE,
            extensions: RegisteredExtensions::none(),
            attestation: ParsedAttestation {
                data: ParsedAttestationData::None,
                metadata: AttestationMetadata::None,
            },
            attestation_format: AttestationFormat::None,
        };

        // Persist it to our fake db.

        // Captured authentication attempt, this client has used the appid extension
        let rsp = r#"
        {
            "id": "z077A6SzdvA3rDRG6tfnTf9TMFVtfLYe1mh27mRXgBZU6TXA_nCJAi6WnLLq1p3d0yj3n62_4yJMu80o4O8kkw",
            "rawId": "z077A6SzdvA3rDRG6tfnTf9TMFVtfLYe1mh27mRXgBZU6TXA_nCJAi6WnLLq1p3d0yj3n62_4yJMu80o4O8kkw",
            "type": "public-key",
            "extensions": {
                "appid": true
            },
            "response": {
                "authenticatorData": "UN6WJxNDzSGdqrQoPbqYbsZdIxtC9vfU9iGe5i1pCTYBAAAAuQ",
                "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJvSF9WcnBZazVMNHBQZGdPcTc5THkyTTdCUHd4V3VzazNLV2Z5VHJoLUk0IiwiY2xpZW50RXh0ZW5zaW9ucyI6eyJhcHBpZCI6Imh0dHBzOi8vdGVzdGluZy5sb2NhbC9hcHAtaWQuanNvbiJ9LCJoYXNoQWxnb3JpdGhtIjoiU0hBLTI1NiIsIm9yaWdpbiI6Imh0dHBzOi8vdGVzdGluZy5sb2NhbCIsInR5cGUiOiJ3ZWJhdXRobi5nZXQifQ",
                "signature": "MEUCIEw2O8LYZj6IKbjP6FuvdcL2MoDBY6LqJWuhteje3H7eAiEAvzRLSg70tkrGnZqpQIZyv-zaizNpCtyr4U3SZ-E2-f4"
            }
        }
        "#;
        let rsp_d: PublicKeyCredential = serde_json::from_str(rsp).unwrap();

        // Now verify it!
        let r = wan.verify_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &zero_chal,
            &cred,
            &Some(String::from("https://testing.local/app-id.json")),
            false,
        );
        trace!("RESULT: {:?}", r);
        assert!(r.is_ok());

        // Captured authentication attempt, this client has NOT used the appid extension, but is providing the appid anyway
        let rsp = r#"
        {
            "id": "z077A6SzdvA3rDRG6tfnTf9TMFVtfLYe1mh27mRXgBZU6TXA_nCJAi6WnLLq1p3d0yj3n62_4yJMu80o4O8kkw",
            "rawId": "z077A6SzdvA3rDRG6tfnTf9TMFVtfLYe1mh27mRXgBZU6TXA_nCJAi6WnLLq1p3d0yj3n62_4yJMu80o4O8kkw",
            "type": "public-key",
            "extensions": {
                "appid": false
            },
            "response": {
                "authenticatorData": "UN6WJxNDzSGdqrQoPbqYbsZdIxtC9vfU9iGe5i1pCTYBAAAAuQ",
                "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJvSF9WcnBZazVMNHBQZGdPcTc5THkyTTdCUHd4V3VzazNLV2Z5VHJoLUk0IiwiY2xpZW50RXh0ZW5zaW9ucyI6eyJhcHBpZCI6Imh0dHBzOi8vdGVzdGluZy5sb2NhbC9hcHAtaWQuanNvbiJ9LCJoYXNoQWxnb3JpdGhtIjoiU0hBLTI1NiIsIm9yaWdpbiI6Imh0dHBzOi8vdGVzdGluZy5sb2NhbCIsInR5cGUiOiJ3ZWJhdXRobi5nZXQifQ",
                "signature": "MEUCIEw2O8LYZj6IKbjP6FuvdcL2MoDBY6LqJWuhteje3H7eAiEAvzRLSg70tkrGnZqpQIZyv-zaizNpCtyr4U3SZ-E2-f4"
            }
        }
        "#;
        let rsp_d: PublicKeyCredential = serde_json::from_str(rsp).unwrap();

        // This will verify against the RPID while the client provided an appid, so it should fail
        let r = wan.verify_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &zero_chal,
            &cred,
            &None,
            false,
        );
        trace!("RESULT: {:?}", r);
        assert!(r.is_err());
    }

    #[test]
    fn test_registration_ipados_5ci() {
        let _ = tracing_subscriber::fmt()
            .with_max_level(tracing::Level::TRACE)
            .try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://172.20.0.141:8443/auth",
            "172.20.0.141",
            vec![Url::parse("https://172.20.0.141:8443").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(
            STANDARD
                .decode("tvR1m+d/ohXrwVxQjMgH8KnovHZ7BRWhZmDN4TVMpNU=")
                .unwrap(),
        );

        let rsp_d = RegisterPublicKeyCredential {
            id: "uZcVDBVS68E_MtAgeQpElJxldF_6cY9sSvbWqx_qRh8wiu42lyRBRmh5yFeD_r9k130dMbFHBHI9RTFgdJQIzQ".to_string(),
            raw_id: Base64UrlSafeData::from(
                STANDARD.decode("uZcVDBVS68E/MtAgeQpElJxldF/6cY9sSvbWqx/qRh8wiu42lyRBRmh5yFeD/r9k130dMbFHBHI9RTFgdJQIzQ==").unwrap()
            ),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(
                    STANDARD.decode("o2NmbXRmcGFja2VkZ2F0dFN0bXSjY2FsZyZjc2lnWEcwRQIhAKAZODmj+uF5qXsDY2NFol3apRjld544KRUpHzwfk5cbAiBnp2gHmamr2xr46ilQuhzIR9BwMlwtxWd6IT2QEYeo7WN4NWOBWQLBMIICvTCCAaWgAwIBAgIEK/F8eDANBgkqhkiG9w0BAQsFADAuMSwwKgYDVQQDEyNZdWJpY28gVTJGIFJvb3QgQ0EgU2VyaWFsIDQ1NzIwMDYzMTAgFw0xNDA4MDEwMDAwMDBaGA8yMDUwMDkwNDAwMDAwMFowbjELMAkGA1UEBhMCU0UxEjAQBgNVBAoMCVl1YmljbyBBQjEiMCAGA1UECwwZQXV0aGVudGljYXRvciBBdHRlc3RhdGlvbjEnMCUGA1UEAwweWXViaWNvIFUyRiBFRSBTZXJpYWwgNzM3MjQ2MzI4MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEdMLHhCPIcS6bSPJZWGb8cECuTN8H13fVha8Ek5nt+pI8vrSflxb59Vp4bDQlH8jzXj3oW1ZwUDjHC6EnGWB5i6NsMGowIgYJKwYBBAGCxAoCBBUxLjMuNi4xLjQuMS40MTQ4Mi4xLjcwEwYLKwYBBAGC5RwCAQEEBAMCAiQwIQYLKwYBBAGC5RwBAQQEEgQQxe9V/62aS5+1gK3rr+Am0DAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4IBAQCLbpN2nXhNbunZANJxAn/Cd+S4JuZsObnUiLnLLS0FPWa01TY8F7oJ8bE+aFa4kTe6NQQfi8+yiZrQ8N+JL4f7gNdQPSrH+r3iFd4SvroDe1jaJO4J9LeiFjmRdcVa+5cqNF4G1fPCofvw9W4lKnObuPakr0x/icdVq1MXhYdUtQk6Zr5mBnc4FhN9qi7DXqLHD5G7ZFUmGwfIcD2+0m1f1mwQS8yRD5+/aDCf3vutwddoi3crtivzyromwbKklR4qHunJ75LGZLZA8pJ/mXnUQ6TTsgRqPvPXgQPbSyGMf2z/DIPbQqCD/Bmc4dj9o6LozheBdDtcZCAjSPTAd/uiaGF1dGhEYXRhWMS3tF916xTswLEZrAO3fy8EzMmvvR8f5wWM7F5+4KJ0ikEAAAACxe9V/62aS5+1gK3rr+Am0ABAuZcVDBVS68E/MtAgeQpElJxldF/6cY9sSvbWqx/qRh8wiu42lyRBRmh5yFeD/r9k130dMbFHBHI9RTFgdJQIzaUBAgMmIAEhWCDCfn9t/BeDFfwG32Ms/owb5hFeBYUcaCmQRauVoRrI8yJYII97t5wYshX4dZ+iRas0vPwaOwYvZ1wTOnVn+QDbCF/E").unwrap()
                ),
                client_data_json: Base64UrlSafeData::from(
                    STANDARD.decode("eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwib3JpZ2luIjoiaHR0cHM6XC9cLzE3Mi4yMC4wLjE0MTo4NDQzIiwiY2hhbGxlbmdlIjoidHZSMW0tZF9vaFhyd1Z4UWpNZ0g4S25vdkhaN0JSV2habURONFRWTXBOVSJ9").unwrap()
                ),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        // Assert this fails when the attestaion is missing.
        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            // This is what introduces the failure!
            Some(&AttestationCaList::default()),
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::MissingAttestationCaList)
        ));

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            // Exclude the matching CA!
            Some(&(APPLE_WEBAUTHN_ROOT_CA_PEM.try_into().unwrap())),
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::AttestationChainNotTrusted(_))
        ));

        // Assert this fails when the attestaion ca is correct, but the aaguid is missing.

        let mut att_ca_builder = AttestationCaListBuilder::new();
        att_ca_builder
            .insert_device_pem(
                YUBICO_U2F_ROOT_CA_SERIAL_457200631_PEM,
                uuid::uuid!("73bb0cd4-e502-49b8-9c6f-b59445bf720b"),
                "yk 5 fips".to_string(),
                Default::default(),
            )
            .expect("Failed to build att ca list");
        let att_ca_list: AttestationCaList = att_ca_builder.build();

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            Some(&att_ca_list),
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::AttestationUntrustedAaguid)
        ));

        let mut att_ca_builder = AttestationCaListBuilder::new();
        att_ca_builder
            .insert_device_pem(
                YUBICO_U2F_ROOT_CA_SERIAL_457200631_PEM,
                uuid::uuid!("c5ef55ff-ad9a-4b9f-b580-adebafe026d0"),
                "yk 5 ci".to_string(),
                Default::default(),
            )
            .expect("Failed to build att ca list");
        let att_ca_list: AttestationCaList = att_ca_builder.build();

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            Some(&att_ca_list),
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        trace!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_deserialise_ipados_5ci() {
        // This is to test migration between the x/y byte array to base64 format.
        let _ = tracing_subscriber::fmt()
            .with_max_level(tracing::Level::TRACE)
            .try_init();
        let ser_cred = r#"{"cred_id":"uZcVDBVS68E_MtAgeQpElJxldF_6cY9sSvbWqx_qRh8wiu42lyRBRmh5yFeD_r9k130dMbFHBHI9RTFgdJQIzQ","cred":{"type_":"ES256","key":{"EC_EC2":{"curve":"SECP256R1","x":[194,126,127,109,252,23,131,21,252,6,223,99,44,254,140,27,230,17,94,5,133,28,104,41,144,69,171,149,161,26,200,243],"y":[143,123,183,156,24,178,21,248,117,159,162,69,171,52,188,252,26,59,6,47,103,92,19,58,117,103,249,0,219,8,95,196]}}},"counter":2,"user_verified":false,"backup_eligible":false,"backup_state":false,"registration_policy":"preferred","extensions":{"cred_protect":"NotRequested","hmac_create_secret":"NotRequested"},"attestation":{"data":{"Basic":["MIICvTCCAaWgAwIBAgIEK_F8eDANBgkqhkiG9w0BAQsFADAuMSwwKgYDVQQDEyNZdWJpY28gVTJGIFJvb3QgQ0EgU2VyaWFsIDQ1NzIwMDYzMTAgFw0xNDA4MDEwMDAwMDBaGA8yMDUwMDkwNDAwMDAwMFowbjELMAkGA1UEBhMCU0UxEjAQBgNVBAoMCVl1YmljbyBBQjEiMCAGA1UECwwZQXV0aGVudGljYXRvciBBdHRlc3RhdGlvbjEnMCUGA1UEAwweWXViaWNvIFUyRiBFRSBTZXJpYWwgNzM3MjQ2MzI4MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEdMLHhCPIcS6bSPJZWGb8cECuTN8H13fVha8Ek5nt-pI8vrSflxb59Vp4bDQlH8jzXj3oW1ZwUDjHC6EnGWB5i6NsMGowIgYJKwYBBAGCxAoCBBUxLjMuNi4xLjQuMS40MTQ4Mi4xLjcwEwYLKwYBBAGC5RwCAQEEBAMCAiQwIQYLKwYBBAGC5RwBAQQEEgQQxe9V_62aS5-1gK3rr-Am0DAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4IBAQCLbpN2nXhNbunZANJxAn_Cd-S4JuZsObnUiLnLLS0FPWa01TY8F7oJ8bE-aFa4kTe6NQQfi8-yiZrQ8N-JL4f7gNdQPSrH-r3iFd4SvroDe1jaJO4J9LeiFjmRdcVa-5cqNF4G1fPCofvw9W4lKnObuPakr0x_icdVq1MXhYdUtQk6Zr5mBnc4FhN9qi7DXqLHD5G7ZFUmGwfIcD2-0m1f1mwQS8yRD5-_aDCf3vutwddoi3crtivzyromwbKklR4qHunJ75LGZLZA8pJ_mXnUQ6TTsgRqPvPXgQPbSyGMf2z_DIPbQqCD_Bmc4dj9o6LozheBdDtcZCAjSPTAd_ui"]},"metadata":"None"},"attestation_format":"Packed"}"#;
        let cred: Credential = serde_json::from_str(ser_cred).unwrap();
        trace!("{:?}", cred);
    }

    #[test]
    fn test_win_hello_attest_none() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://etools-dev.example.com:8080/auth",
            "etools-dev.example.com",
            vec![Url::parse("https://etools-dev.example.com:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );
        let chal = Challenge::new(vec![
            21, 9, 50, 208, 90, 167, 153, 94, 74, 98, 161, 84, 247, 161, 61, 104, 10, 82, 33, 27,
            99, 94, 34, 156, 84, 85, 31, 240, 9, 188, 136, 52,
        ]);

        let rsp_d = RegisterPublicKeyCredential {
            id: "KwlEDOBCBc9P1YU3NWihYLCeY-I9KGMhPap9vwHbVoI".to_string(),
            raw_id: Base64UrlSafeData::from(vec![
                43, 9, 68, 12, 224, 66, 5, 207, 79, 213, 133, 55, 53, 104, 161, 96, 176, 158, 99,
                226, 61, 40, 99, 33, 61, 170, 125, 191, 1, 219, 86, 130,
            ]),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(vec![
                    163, 99, 102, 109, 116, 100, 110, 111, 110, 101, 103, 97, 116, 116, 83, 116,
                    109, 116, 160, 104, 97, 117, 116, 104, 68, 97, 116, 97, 89, 1, 103, 108, 41,
                    129, 232, 231, 178, 172, 146, 198, 102, 0, 255, 160, 250, 221, 227, 137, 40,
                    196, 142, 208, 221, 115, 246, 47, 198, 69, 45, 165, 107, 42, 27, 69, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 43, 9, 68, 12, 224,
                    66, 5, 207, 79, 213, 133, 55, 53, 104, 161, 96, 176, 158, 99, 226, 61, 40, 99,
                    33, 61, 170, 125, 191, 1, 219, 86, 130, 164, 1, 3, 3, 57, 1, 0, 32, 89, 1, 0,
                    166, 163, 131, 233, 97, 64, 136, 207, 111, 39, 80, 80, 230, 19, 46, 59, 12,
                    247, 151, 113, 167, 157, 140, 198, 227, 168, 159, 211, 232, 112, 116, 209, 54,
                    148, 26, 156, 56, 88, 56, 27, 116, 102, 237, 88, 99, 81, 65, 79, 133, 242, 192,
                    25, 28, 45, 116, 131, 129, 253, 185, 91, 35, 129, 35, 193, 44, 64, 86, 87, 137,
                    44, 19, 74, 239, 72, 178, 243, 11, 195, 135, 194, 216, 109, 62, 84, 172, 16,
                    182, 82, 140, 170, 1, 255, 91, 80, 73, 100, 1, 117, 61, 148, 179, 95, 199, 169,
                    228, 244, 174, 69, 54, 185, 15, 107, 5, 0, 110, 155, 28, 243, 114, 32, 176,
                    220, 93, 196, 172, 158, 22, 3, 154, 18, 148, 20, 132, 94, 166, 45, 24, 27, 8,
                    255, 108, 31, 230, 196, 122, 125, 240, 215, 219, 118, 80, 224, 146, 92, 80,
                    219, 91, 211, 88, 45, 28, 133, 135, 83, 244, 212, 29, 121, 132, 104, 189, 3,
                    98, 42, 180, 10, 249, 232, 59, 172, 204, 109, 64, 206, 139, 76, 247, 230, 40,
                    36, 71, 79, 11, 139, 84, 211, 153, 125, 108, 108, 55, 195, 205, 5, 90, 248, 72,
                    42, 94, 40, 136, 193, 89, 3, 102, 109, 30, 65, 117, 76, 103, 150, 4, 44, 155,
                    104, 207, 126, 92, 16, 161, 175, 223, 119, 246, 169, 127, 72, 13, 83, 129, 12,
                    164, 102, 42, 141, 173, 102, 140, 52, 57, 43, 115, 12, 238, 89, 33, 67, 1, 0,
                    1,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 99, 114, 101, 97, 116, 101, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110,
                    103, 101, 34, 58, 34, 70, 81, 107, 121, 48, 70, 113, 110, 109, 86, 53, 75, 89,
                    113, 70, 85, 57, 54, 69, 57, 97, 65, 112, 83, 73, 82, 116, 106, 88, 105, 75,
                    99, 86, 70, 85, 102, 56, 65, 109, 56, 105, 68, 81, 34, 44, 34, 111, 114, 105,
                    103, 105, 110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 101, 116, 111,
                    111, 108, 115, 45, 100, 101, 118, 46, 101, 120, 97, 109, 112, 108, 101, 46, 99,
                    111, 109, 58, 56, 48, 56, 48, 34, 44, 34, 99, 114, 111, 115, 115, 79, 114, 105,
                    103, 105, 110, 34, 58, 102, 97, 108, 115, 101, 125,
                ]),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::RS256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            true,
        );
        trace!("{:?}", result);
        assert!(result.is_ok());
        let cred = result.unwrap();

        let chal = Challenge::new(vec![
            189, 116, 126, 107, 74, 29, 210, 181, 99, 178, 173, 214, 166, 212, 124, 219, 29, 169,
            9, 58, 26, 27, 120, 246, 87, 173, 169, 210, 241, 153, 150, 189,
        ]);

        let rsp_d = PublicKeyCredential {
            id: "KwlEDOBCBc9P1YU3NWihYLCeY-I9KGMhPap9vwHbVoI".to_string(),
            raw_id: Base64UrlSafeData::from(vec![
                43, 9, 68, 12, 224, 66, 5, 207, 79, 213, 133, 55, 53, 104, 161, 96, 176, 158, 99,
                226, 61, 40, 99, 33, 61, 170, 125, 191, 1, 219, 86, 130,
            ]),
            response: AuthenticatorAssertionResponseRaw {
                authenticator_data: Base64UrlSafeData::from(vec![
                    108, 41, 129, 232, 231, 178, 172, 146, 198, 102, 0, 255, 160, 250, 221, 227,
                    137, 40, 196, 142, 208, 221, 115, 246, 47, 198, 69, 45, 165, 107, 42, 27, 5, 0,
                    0, 0, 1,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 103, 101, 116, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110, 103, 101, 34,
                    58, 34, 118, 88, 82, 45, 97, 48, 111, 100, 48, 114, 86, 106, 115, 113, 51, 87,
                    112, 116, 82, 56, 50, 120, 50, 112, 67, 84, 111, 97, 71, 51, 106, 50, 86, 54,
                    50, 112, 48, 118, 71, 90, 108, 114, 48, 34, 44, 34, 111, 114, 105, 103, 105,
                    110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 101, 116, 111, 111, 108,
                    115, 45, 100, 101, 118, 46, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109,
                    58, 56, 48, 56, 48, 34, 44, 34, 99, 114, 111, 115, 115, 79, 114, 105, 103, 105,
                    110, 34, 58, 102, 97, 108, 115, 101, 125,
                ]),
                signature: Base64UrlSafeData::from(vec![
                    77, 253, 152, 83, 184, 198, 5, 16, 68, 51, 178, 5, 228, 20, 148, 168, 182, 3,
                    201, 59, 162, 181, 96, 221, 67, 136, 230, 61, 252, 0, 38, 244, 143, 98, 100,
                    14, 226, 223, 234, 58, 72, 9, 230, 190, 0, 189, 176, 101, 172, 176, 146, 25,
                    221, 117, 79, 13, 176, 99, 208, 211, 135, 15, 60, 245, 106, 232, 195, 215, 37,
                    70, 136, 198, 25, 186, 156, 226, 77, 216, 85, 100, 139, 73, 73, 173, 210, 244,
                    116, 84, 108, 180, 138, 115, 15, 187, 140, 198, 110, 218, 78, 238, 99, 131,
                    210, 229, 242, 184, 133, 219, 177, 235, 96, 187, 143, 82, 243, 88, 120, 214,
                    182, 118, 88, 198, 157, 233, 83, 206, 165, 187, 111, 83, 211, 68, 147, 137,
                    176, 28, 173, 36, 66, 87, 225, 252, 195, 101, 181, 44, 119, 198, 48, 210, 186,
                    188, 190, 20, 78, 14, 49, 67, 144, 131, 76, 85, 70, 95, 130, 137, 132, 168, 33,
                    196, 113, 83, 59, 38, 46, 1, 167, 107, 200, 168, 242, 6, 106, 141, 203, 123,
                    203, 50, 69, 173, 6, 183, 117, 118, 229, 188, 39, 120, 188, 48, 54, 117, 223,
                    15, 153, 122, 4, 24, 218, 56, 251, 173, 166, 113, 240, 231, 175, 21, 28, 228,
                    248, 10, 1, 73, 222, 52, 57, 72, 51, 44, 131, 206, 4, 243, 66, 100, 61, 113,
                    237, 221, 115, 182, 37, 187, 29, 250, 103, 178, 104, 69, 153, 47, 212, 76, 200,
                    242,
                ]),
                user_handle: Some(Base64UrlSafeData::from(vec![109, 99, 104, 97, 110])),
            },
            extensions: AuthenticationExtensionsClientOutputs::default(),
            type_: "public-key".to_string(),
        };

        let r = wan.verify_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &cred,
            &None,
            false,
        );
        trace!("RESULT: {:?}", r);
        assert!(r.is_ok());
    }

    #[test]
    fn test_win_hello_attest_tpm() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://etools-dev.example.com:8080/auth",
            "etools-dev.example.com",
            vec![Url::parse("https://etools-dev.example.com:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(vec![
            34, 92, 189, 180, 54, 92, 96, 184, 1, 200, 155, 91, 42, 168, 156, 94, 254, 223, 49,
            169, 171, 179, 2, 71, 90, 123, 180, 244, 37, 182, 17, 52,
        ]);

        let rsp_d = RegisterPublicKeyCredential {
            id: "0_n4aTCbomLUQXr07c7Ea-J0iNvdYmW0bUGuN6-ceGA".to_string(),
            raw_id: Base64UrlSafeData::from(vec![
                211, 249, 248, 105, 48, 155, 162, 98, 212, 65, 122, 244, 237, 206, 196, 107, 226,
                116, 136, 219, 221, 98, 101, 180, 109, 65, 174, 55, 175, 156, 120, 96,
            ]),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(vec![
                    163, 99, 102, 109, 116, 99, 116, 112, 109, 103, 97, 116, 116, 83, 116, 109,
                    116, 166, 99, 97, 108, 103, 57, 255, 254, 99, 115, 105, 103, 89, 1, 0, 5, 3,
                    162, 216, 151, 57, 210, 103, 145, 121, 161, 186, 63, 232, 221, 255, 89, 37, 17,
                    59, 155, 241, 77, 30, 35, 201, 30, 140, 84, 214, 250, 185, 47, 248, 58, 89,
                    177, 187, 231, 202, 220, 45, 167, 126, 243, 194, 94, 33, 39, 205, 163, 51, 40,
                    171, 35, 118, 196, 244, 247, 143, 166, 193, 223, 94, 244, 157, 121, 220, 22,
                    94, 163, 15, 151, 223, 214, 131, 105, 202, 40, 16, 176, 11, 154, 102, 100, 212,
                    174, 103, 166, 92, 90, 154, 224, 20, 165, 106, 127, 53, 91, 230, 217, 199, 172,
                    195, 203, 242, 41, 158, 64, 252, 65, 9, 155, 160, 63, 40, 94, 94, 64, 145, 173,
                    71, 85, 173, 2, 199, 18, 148, 88, 223, 93, 154, 203, 197, 170, 142, 35, 249,
                    146, 107, 146, 2, 14, 54, 39, 151, 181, 10, 176, 216, 117, 25, 196, 2, 205,
                    159, 140, 155, 56, 89, 87, 31, 135, 93, 97, 78, 95, 176, 228, 72, 237, 130,
                    171, 23, 66, 232, 35, 115, 218, 105, 168, 6, 253, 121, 161, 129, 44, 78, 252,
                    44, 11, 23, 172, 66, 37, 214, 113, 128, 28, 33, 209, 66, 34, 32, 196, 153, 80,
                    87, 243, 162, 7, 25, 62, 252, 243, 174, 31, 168, 98, 123, 100, 2, 143, 134, 36,
                    154, 236, 18, 128, 175, 185, 189, 177, 51, 53, 216, 190, 43, 63, 35, 84, 14,
                    64, 249, 23, 9, 125, 147, 160, 176, 137, 30, 174, 245, 148, 189, 99, 118, 101,
                    114, 99, 50, 46, 48, 99, 120, 53, 99, 130, 89, 5, 189, 48, 130, 5, 185, 48,
                    130, 3, 161, 160, 3, 2, 1, 2, 2, 16, 88, 191, 48, 69, 71, 45, 69, 233, 150,
                    144, 71, 177, 166, 190, 225, 202, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1,
                    1, 11, 5, 0, 48, 66, 49, 64, 48, 62, 6, 3, 85, 4, 3, 19, 55, 78, 67, 85, 45,
                    73, 78, 84, 67, 45, 75, 69, 89, 73, 68, 45, 54, 67, 65, 57, 68, 70, 54, 50, 65,
                    49, 65, 65, 69, 50, 51, 69, 48, 70, 69, 66, 55, 67, 51, 70, 53, 69, 66, 56, 69,
                    54, 49, 69, 67, 65, 67, 49, 55, 67, 66, 55, 48, 30, 23, 13, 50, 48, 48, 56, 49,
                    49, 49, 54, 50, 50, 49, 54, 90, 23, 13, 50, 53, 48, 51, 50, 49, 50, 48, 51, 48,
                    48, 50, 90, 48, 0, 48, 130, 1, 34, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1,
                    1, 1, 5, 0, 3, 130, 1, 15, 0, 48, 130, 1, 10, 2, 130, 1, 1, 0, 197, 166, 58,
                    190, 204, 104, 240, 65, 135, 183, 96, 7, 143, 26, 55, 77, 107, 12, 171, 56, 2,
                    145, 240, 201, 220, 75, 161, 201, 223, 24, 207, 126, 10, 118, 48, 201, 191, 6,
                    187, 227, 178, 255, 229, 252, 127, 199, 215, 76, 221, 180, 123, 111, 178, 141,
                    58, 235, 87, 27, 29, 24, 52, 235, 235, 181, 241, 28, 109, 223, 48, 137, 54, 21,
                    113, 155, 105, 39, 210, 237, 238, 172, 146, 195, 173, 170, 137, 201, 36, 212,
                    77, 179, 246, 142, 19, 198, 242, 48, 161, 199, 209, 113, 228, 182, 205, 115, 8,
                    29, 255, 6, 29, 87, 118, 157, 115, 116, 171, 64, 105, 248, 91, 128, 220, 98,
                    209, 126, 157, 177, 227, 101, 26, 26, 239, 72, 162, 135, 177, 177, 130, 16,
                    239, 79, 140, 1, 29, 26, 38, 57, 7, 96, 218, 94, 110, 49, 251, 102, 130, 28,
                    128, 227, 105, 117, 184, 13, 29, 229, 137, 151, 164, 116, 179, 101, 134, 253,
                    159, 165, 90, 245, 195, 156, 105, 87, 147, 61, 219, 46, 29, 191, 252, 201, 117,
                    54, 207, 6, 157, 96, 161, 26, 39, 172, 229, 85, 225, 172, 220, 252, 242, 129,
                    34, 7, 227, 8, 7, 112, 42, 34, 73, 125, 6, 241, 100, 14, 214, 125, 179, 63,
                    106, 150, 111, 19, 235, 59, 24, 141, 217, 140, 125, 91, 73, 152, 206, 174, 0,
                    237, 72, 250, 207, 138, 119, 143, 203, 206, 115, 97, 89, 211, 219, 245, 2, 3,
                    1, 0, 1, 163, 130, 1, 235, 48, 130, 1, 231, 48, 14, 6, 3, 85, 29, 15, 1, 1,
                    255, 4, 4, 3, 2, 7, 128, 48, 12, 6, 3, 85, 29, 19, 1, 1, 255, 4, 2, 48, 0, 48,
                    109, 6, 3, 85, 29, 32, 1, 1, 255, 4, 99, 48, 97, 48, 95, 6, 9, 43, 6, 1, 4, 1,
                    130, 55, 21, 31, 48, 82, 48, 80, 6, 8, 43, 6, 1, 5, 5, 7, 2, 2, 48, 68, 30, 66,
                    0, 84, 0, 67, 0, 80, 0, 65, 0, 32, 0, 32, 0, 84, 0, 114, 0, 117, 0, 115, 0,
                    116, 0, 101, 0, 100, 0, 32, 0, 32, 0, 80, 0, 108, 0, 97, 0, 116, 0, 102, 0,
                    111, 0, 114, 0, 109, 0, 32, 0, 32, 0, 73, 0, 100, 0, 101, 0, 110, 0, 116, 0,
                    105, 0, 116, 0, 121, 48, 16, 6, 3, 85, 29, 37, 4, 9, 48, 7, 6, 5, 103, 129, 5,
                    8, 3, 48, 80, 6, 3, 85, 29, 17, 1, 1, 255, 4, 70, 48, 68, 164, 66, 48, 64, 49,
                    22, 48, 20, 6, 5, 103, 129, 5, 2, 1, 12, 11, 105, 100, 58, 52, 57, 52, 69, 53,
                    52, 52, 51, 49, 14, 48, 12, 6, 5, 103, 129, 5, 2, 2, 12, 3, 83, 80, 84, 49, 22,
                    48, 20, 6, 5, 103, 129, 5, 2, 3, 12, 11, 105, 100, 58, 48, 48, 48, 50, 48, 48,
                    48, 48, 48, 31, 6, 3, 85, 29, 35, 4, 24, 48, 22, 128, 20, 147, 147, 77, 66, 14,
                    183, 179, 161, 2, 110, 122, 113, 35, 6, 16, 82, 232, 88, 88, 179, 48, 29, 6, 3,
                    85, 29, 14, 4, 22, 4, 20, 168, 251, 63, 173, 250, 64, 138, 217, 186, 126, 231,
                    77, 242, 159, 198, 195, 60, 109, 251, 231, 48, 129, 179, 6, 8, 43, 6, 1, 5, 5,
                    7, 1, 1, 4, 129, 166, 48, 129, 163, 48, 129, 160, 6, 8, 43, 6, 1, 5, 5, 7, 48,
                    2, 134, 129, 147, 104, 116, 116, 112, 58, 47, 47, 97, 122, 99, 115, 112, 114,
                    111, 100, 110, 99, 117, 97, 105, 107, 112, 117, 98, 108, 105, 115, 104, 46, 98,
                    108, 111, 98, 46, 99, 111, 114, 101, 46, 119, 105, 110, 100, 111, 119, 115, 46,
                    110, 101, 116, 47, 110, 99, 117, 45, 105, 110, 116, 99, 45, 107, 101, 121, 105,
                    100, 45, 54, 99, 97, 57, 100, 102, 54, 50, 97, 49, 97, 97, 101, 50, 51, 101,
                    48, 102, 101, 98, 55, 99, 51, 102, 53, 101, 98, 56, 101, 54, 49, 101, 99, 97,
                    99, 49, 55, 99, 98, 55, 47, 100, 56, 101, 48, 50, 49, 56, 101, 45, 55, 55, 101,
                    98, 45, 52, 51, 98, 56, 45, 97, 57, 56, 49, 45, 51, 48, 53, 99, 101, 99, 99,
                    53, 99, 98, 97, 54, 46, 99, 101, 114, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13,
                    1, 1, 11, 5, 0, 3, 130, 2, 1, 0, 4, 128, 111, 190, 0, 94, 133, 167, 0, 61, 237,
                    232, 184, 182, 255, 238, 77, 189, 198, 248, 63, 5, 5, 202, 60, 98, 125, 121,
                    175, 177, 82, 252, 85, 154, 80, 32, 167, 198, 224, 128, 251, 145, 5, 32, 101,
                    218, 186, 38, 255, 178, 63, 167, 51, 205, 62, 195, 167, 219, 144, 6, 11, 70,
                    14, 59, 177, 178, 116, 254, 131, 199, 231, 75, 204, 62, 116, 231, 40, 47, 112,
                    138, 24, 194, 154, 46, 30, 25, 149, 75, 139, 119, 164, 65, 187, 215, 24, 139,
                    160, 76, 210, 124, 16, 77, 27, 225, 70, 251, 137, 3, 176, 229, 248, 51, 108,
                    163, 125, 36, 240, 181, 104, 49, 102, 42, 44, 172, 14, 255, 46, 131, 47, 7,
                    180, 126, 84, 104, 151, 134, 42, 81, 159, 58, 126, 37, 224, 145, 122, 27, 111,
                    213, 236, 124, 97, 181, 112, 75, 29, 33, 34, 7, 210, 170, 139, 63, 18, 193, 98,
                    94, 186, 138, 225, 215, 44, 242, 91, 77, 201, 60, 66, 4, 27, 22, 85, 228, 223,
                    59, 42, 242, 163, 164, 219, 75, 174, 91, 118, 115, 29, 216, 53, 37, 124, 161,
                    194, 15, 117, 147, 50, 98, 205, 196, 137, 1, 244, 26, 124, 236, 181, 184, 5,
                    98, 64, 191, 209, 189, 64, 0, 11, 214, 153, 64, 2, 36, 116, 237, 238, 124, 47,
                    47, 182, 246, 20, 105, 12, 168, 188, 192, 215, 26, 228, 86, 69, 212, 42, 69,
                    121, 238, 73, 155, 154, 133, 203, 30, 108, 94, 184, 214, 91, 67, 79, 22, 118,
                    63, 100, 249, 23, 90, 142, 72, 94, 238, 91, 154, 32, 191, 51, 192, 44, 197,
                    212, 173, 119, 159, 156, 71, 96, 239, 37, 68, 73, 247, 102, 88, 203, 172, 113,
                    250, 74, 247, 129, 79, 19, 235, 145, 95, 158, 214, 44, 38, 28, 244, 218, 86,
                    202, 93, 73, 196, 209, 133, 138, 77, 42, 58, 221, 99, 112, 13, 73, 47, 22, 108,
                    162, 144, 47, 36, 208, 114, 146, 87, 77, 24, 78, 66, 148, 86, 91, 169, 104,
                    104, 106, 137, 126, 172, 10, 213, 37, 25, 179, 175, 253, 243, 212, 175, 240,
                    103, 8, 180, 190, 108, 198, 199, 40, 171, 227, 161, 232, 53, 147, 109, 244, 93,
                    113, 237, 64, 179, 160, 78, 35, 34, 8, 136, 179, 185, 176, 219, 4, 198, 38,
                    175, 6, 12, 227, 55, 168, 192, 122, 115, 119, 95, 205, 244, 105, 116, 238, 137,
                    228, 32, 4, 9, 219, 246, 49, 131, 190, 64, 37, 85, 108, 239, 164, 173, 90, 254,
                    146, 255, 252, 188, 232, 40, 184, 108, 69, 153, 81, 182, 17, 174, 194, 52, 246,
                    178, 77, 47, 50, 167, 56, 17, 83, 31, 65, 119, 143, 160, 113, 254, 71, 33, 166,
                    88, 53, 128, 195, 6, 193, 50, 144, 78, 242, 155, 234, 231, 20, 144, 132, 177,
                    159, 161, 94, 154, 205, 133, 78, 20, 214, 141, 230, 33, 115, 192, 148, 87, 151,
                    95, 71, 175, 89, 6, 240, 48, 130, 6, 236, 48, 130, 4, 212, 160, 3, 2, 1, 2, 2,
                    19, 51, 0, 0, 2, 113, 82, 34, 55, 131, 10, 123, 56, 174, 0, 0, 0, 0, 2, 113,
                    48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 11, 5, 0, 48, 129, 140, 49, 11,
                    48, 9, 6, 3, 85, 4, 6, 19, 2, 85, 83, 49, 19, 48, 17, 6, 3, 85, 4, 8, 19, 10,
                    87, 97, 115, 104, 105, 110, 103, 116, 111, 110, 49, 16, 48, 14, 6, 3, 85, 4, 7,
                    19, 7, 82, 101, 100, 109, 111, 110, 100, 49, 30, 48, 28, 6, 3, 85, 4, 10, 19,
                    21, 77, 105, 99, 114, 111, 115, 111, 102, 116, 32, 67, 111, 114, 112, 111, 114,
                    97, 116, 105, 111, 110, 49, 54, 48, 52, 6, 3, 85, 4, 3, 19, 45, 77, 105, 99,
                    114, 111, 115, 111, 102, 116, 32, 84, 80, 77, 32, 82, 111, 111, 116, 32, 67,
                    101, 114, 116, 105, 102, 105, 99, 97, 116, 101, 32, 65, 117, 116, 104, 111,
                    114, 105, 116, 121, 32, 50, 48, 49, 52, 48, 30, 23, 13, 49, 57, 48, 51, 50, 49,
                    50, 48, 51, 48, 48, 50, 90, 23, 13, 50, 53, 48, 51, 50, 49, 50, 48, 51, 48, 48,
                    50, 90, 48, 66, 49, 64, 48, 62, 6, 3, 85, 4, 3, 19, 55, 78, 67, 85, 45, 73, 78,
                    84, 67, 45, 75, 69, 89, 73, 68, 45, 54, 67, 65, 57, 68, 70, 54, 50, 65, 49, 65,
                    65, 69, 50, 51, 69, 48, 70, 69, 66, 55, 67, 51, 70, 53, 69, 66, 56, 69, 54, 49,
                    69, 67, 65, 67, 49, 55, 67, 66, 55, 48, 130, 2, 34, 48, 13, 6, 9, 42, 134, 72,
                    134, 247, 13, 1, 1, 1, 5, 0, 3, 130, 2, 15, 0, 48, 130, 2, 10, 2, 130, 2, 1, 0,
                    152, 43, 107, 173, 177, 53, 163, 163, 93, 154, 248, 108, 222, 80, 5, 122, 87,
                    236, 252, 225, 50, 52, 121, 17, 29, 232, 18, 63, 7, 156, 177, 34, 151, 214, 92,
                    55, 149, 204, 232, 129, 50, 154, 105, 128, 221, 190, 157, 193, 52, 48, 65, 151,
                    90, 250, 48, 160, 25, 134, 46, 36, 77, 126, 48, 129, 230, 125, 172, 189, 156,
                    247, 147, 31, 239, 20, 230, 78, 4, 146, 123, 54, 173, 175, 211, 248, 18, 125,
                    83, 110, 37, 67, 147, 152, 0, 121, 176, 166, 87, 248, 31, 3, 155, 235, 53, 134,
                    8, 105, 212, 244, 239, 170, 41, 94, 183, 81, 143, 34, 193, 123, 125, 187, 48,
                    149, 59, 99, 240, 15, 38, 108, 172, 200, 222, 70, 62, 98, 80, 163, 32, 19, 26,
                    181, 191, 156, 139, 248, 190, 110, 129, 56, 196, 50, 16, 89, 143, 150, 41, 172,
                    239, 136, 65, 145, 0, 93, 222, 226, 117, 208, 183, 116, 85, 166, 93, 247, 23,
                    39, 167, 130, 47, 73, 113, 26, 102, 197, 100, 212, 176, 34, 143, 98, 105, 5,
                    206, 194, 120, 190, 201, 49, 102, 199, 25, 161, 230, 11, 189, 87, 188, 102,
                    171, 44, 55, 193, 180, 208, 172, 250, 214, 194, 36, 148, 113, 206, 80, 159,
                    124, 135, 247, 246, 51, 10, 194, 204, 232, 44, 33, 64, 183, 63, 209, 225, 72,
                    195, 193, 71, 101, 174, 241, 42, 217, 92, 214, 117, 199, 101, 75, 42, 145, 145,
                    187, 113, 150, 138, 28, 61, 122, 159, 86, 152, 41, 83, 65, 80, 158, 165, 195,
                    96, 255, 135, 34, 90, 161, 69, 173, 74, 198, 147, 96, 85, 40, 100, 128, 191,
                    135, 11, 27, 86, 149, 149, 18, 103, 182, 110, 255, 71, 47, 227, 240, 14, 66,
                    137, 251, 211, 221, 191, 34, 157, 152, 230, 121, 195, 41, 148, 176, 219, 134,
                    62, 178, 181, 89, 7, 166, 111, 81, 85, 222, 85, 218, 96, 48, 120, 135, 99, 119,
                    60, 170, 236, 34, 41, 173, 19, 91, 140, 28, 220, 20, 140, 71, 236, 117, 13,
                    209, 248, 147, 130, 77, 125, 11, 109, 142, 43, 95, 221, 245, 154, 72, 250, 152,
                    36, 107, 77, 175, 133, 247, 233, 77, 225, 123, 53, 217, 16, 39, 218, 44, 7, 97,
                    89, 15, 241, 7, 15, 186, 204, 227, 132, 181, 120, 62, 216, 232, 84, 45, 142,
                    241, 86, 209, 254, 255, 208, 45, 88, 242, 239, 198, 31, 54, 159, 135, 142, 17,
                    52, 142, 58, 126, 81, 118, 231, 23, 209, 48, 11, 80, 194, 124, 248, 205, 80,
                    187, 12, 166, 123, 89, 175, 201, 212, 239, 172, 77, 151, 107, 127, 92, 161, 37,
                    246, 209, 253, 166, 8, 230, 153, 14, 54, 111, 173, 212, 8, 42, 60, 177, 191,
                    97, 130, 28, 51, 178, 40, 129, 46, 179, 24, 45, 26, 25, 59, 61, 94, 4, 145,
                    149, 42, 63, 49, 247, 136, 126, 5, 206, 102, 177, 28, 26, 86, 148, 35, 2, 3, 1,
                    0, 1, 163, 130, 1, 142, 48, 130, 1, 138, 48, 14, 6, 3, 85, 29, 15, 1, 1, 255,
                    4, 4, 3, 2, 2, 132, 48, 27, 6, 3, 85, 29, 37, 4, 20, 48, 18, 6, 9, 43, 6, 1, 4,
                    1, 130, 55, 21, 36, 6, 5, 103, 129, 5, 8, 3, 48, 22, 6, 3, 85, 29, 32, 4, 15,
                    48, 13, 48, 11, 6, 9, 43, 6, 1, 4, 1, 130, 55, 21, 31, 48, 18, 6, 3, 85, 29,
                    19, 1, 1, 255, 4, 8, 48, 6, 1, 1, 255, 2, 1, 0, 48, 29, 6, 3, 85, 29, 14, 4,
                    22, 4, 20, 147, 147, 77, 66, 14, 183, 179, 161, 2, 110, 122, 113, 35, 6, 16,
                    82, 232, 88, 88, 179, 48, 31, 6, 3, 85, 29, 35, 4, 24, 48, 22, 128, 20, 122,
                    140, 10, 206, 47, 72, 98, 23, 226, 148, 209, 174, 85, 193, 82, 236, 113, 116,
                    164, 86, 48, 112, 6, 3, 85, 29, 31, 4, 105, 48, 103, 48, 101, 160, 99, 160, 97,
                    134, 95, 104, 116, 116, 112, 58, 47, 47, 119, 119, 119, 46, 109, 105, 99, 114,
                    111, 115, 111, 102, 116, 46, 99, 111, 109, 47, 112, 107, 105, 111, 112, 115,
                    47, 99, 114, 108, 47, 77, 105, 99, 114, 111, 115, 111, 102, 116, 37, 50, 48,
                    84, 80, 77, 37, 50, 48, 82, 111, 111, 116, 37, 50, 48, 67, 101, 114, 116, 105,
                    102, 105, 99, 97, 116, 101, 37, 50, 48, 65, 117, 116, 104, 111, 114, 105, 116,
                    121, 37, 50, 48, 50, 48, 49, 52, 46, 99, 114, 108, 48, 125, 6, 8, 43, 6, 1, 5,
                    5, 7, 1, 1, 4, 113, 48, 111, 48, 109, 6, 8, 43, 6, 1, 5, 5, 7, 48, 2, 134, 97,
                    104, 116, 116, 112, 58, 47, 47, 119, 119, 119, 46, 109, 105, 99, 114, 111, 115,
                    111, 102, 116, 46, 99, 111, 109, 47, 112, 107, 105, 111, 112, 115, 47, 99, 101,
                    114, 116, 115, 47, 77, 105, 99, 114, 111, 115, 111, 102, 116, 37, 50, 48, 84,
                    80, 77, 37, 50, 48, 82, 111, 111, 116, 37, 50, 48, 67, 101, 114, 116, 105, 102,
                    105, 99, 97, 116, 101, 37, 50, 48, 65, 117, 116, 104, 111, 114, 105, 116, 121,
                    37, 50, 48, 50, 48, 49, 52, 46, 99, 114, 116, 48, 13, 6, 9, 42, 134, 72, 134,
                    247, 13, 1, 1, 11, 5, 0, 3, 130, 2, 1, 0, 73, 235, 166, 7, 16, 89, 131, 50, 67,
                    31, 113, 176, 9, 16, 209, 146, 232, 124, 220, 236, 23, 249, 16, 213, 246, 244,
                    231, 147, 248, 141, 93, 158, 160, 222, 177, 160, 115, 201, 16, 11, 228, 151,
                    21, 209, 62, 191, 38, 153, 95, 178, 20, 202, 150, 24, 170, 85, 100, 155, 108,
                    120, 203, 242, 149, 237, 71, 252, 71, 149, 245, 18, 222, 155, 246, 56, 226,
                    116, 245, 175, 196, 187, 121, 2, 212, 117, 193, 222, 154, 201, 133, 16, 232,
                    171, 149, 255, 214, 198, 212, 197, 65, 34, 27, 55, 16, 54, 91, 251, 95, 52,
                    141, 113, 235, 119, 147, 78, 1, 254, 195, 123, 240, 11, 79, 183, 139, 167, 223,
                    99, 172, 242, 229, 252, 48, 126, 146, 1, 170, 111, 216, 195, 26, 9, 183, 178,
                    32, 197, 94, 57, 33, 1, 165, 51, 121, 63, 4, 53, 36, 195, 106, 69, 23, 244, 74,
                    0, 52, 93, 45, 232, 15, 144, 228, 162, 61, 32, 73, 156, 147, 11, 69, 235, 123,
                    172, 207, 162, 228, 234, 160, 234, 193, 35, 189, 70, 229, 126, 3, 63, 178, 15,
                    224, 235, 103, 203, 74, 37, 37, 146, 94, 43, 123, 179, 63, 216, 150, 144, 199,
                    224, 255, 121, 132, 38, 60, 0, 171, 31, 236, 168, 254, 171, 146, 116, 99, 43,
                    235, 186, 249, 176, 135, 195, 160, 51, 39, 252, 205, 76, 22, 189, 141, 240,
                    196, 2, 116, 193, 211, 79, 70, 63, 14, 37, 53, 170, 224, 243, 135, 251, 85,
                    142, 154, 99, 122, 59, 0, 96, 215, 6, 202, 198, 137, 50, 122, 35, 194, 17, 128,
                    215, 129, 249, 220, 85, 224, 26, 24, 8, 200, 198, 13, 105, 32, 81, 8, 34, 198,
                    33, 222, 79, 161, 60, 167, 105, 246, 195, 242, 5, 126, 69, 23, 54, 78, 166,
                    185, 253, 107, 152, 165, 14, 8, 158, 205, 81, 113, 18, 61, 101, 94, 9, 36, 203,
                    232, 130, 211, 230, 45, 209, 3, 100, 5, 159, 67, 152, 26, 95, 188, 125, 92,
                    141, 251, 62, 72, 40, 203, 116, 89, 14, 141, 8, 120, 232, 19, 235, 85, 35, 101,
                    24, 247, 149, 197, 215, 100, 22, 37, 144, 62, 173, 79, 123, 198, 63, 136, 236,
                    81, 242, 90, 231, 189, 41, 204, 131, 14, 150, 67, 108, 88, 123, 210, 157, 216,
                    251, 32, 193, 91, 82, 3, 107, 199, 180, 155, 243, 12, 23, 77, 162, 231, 227,
                    120, 72, 35, 94, 105, 168, 102, 35, 27, 0, 203, 104, 19, 212, 75, 177, 173, 38,
                    68, 156, 147, 228, 80, 215, 121, 250, 163, 49, 245, 155, 2, 15, 160, 49, 117,
                    74, 100, 43, 119, 37, 26, 23, 96, 188, 144, 155, 211, 185, 166, 123, 250, 211,
                    242, 193, 122, 67, 159, 35, 66, 33, 153, 122, 233, 160, 181, 188, 114, 250, 70,
                    165, 98, 31, 165, 84, 126, 45, 106, 164, 221, 57, 100, 151, 23, 81, 46, 118,
                    251, 43, 100, 201, 204, 121, 103, 112, 117, 98, 65, 114, 101, 97, 89, 1, 54, 0,
                    1, 0, 11, 0, 6, 4, 114, 0, 32, 157, 255, 203, 243, 108, 56, 58, 230, 153, 251,
                    152, 104, 220, 109, 203, 137, 215, 21, 56, 132, 190, 40, 3, 146, 44, 18, 65,
                    88, 191, 173, 34, 174, 0, 16, 0, 16, 8, 0, 0, 0, 0, 0, 1, 0, 220, 20, 243, 114,
                    251, 142, 90, 236, 17, 204, 181, 223, 8, 72, 230, 209, 122, 44, 90, 55, 96,
                    134, 69, 16, 125, 139, 112, 81, 154, 230, 133, 211, 129, 37, 75, 208, 222, 70,
                    210, 239, 209, 188, 152, 93, 222, 222, 154, 169, 217, 160, 90, 243, 135, 151,
                    25, 87, 240, 178, 106, 119, 150, 89, 23, 223, 158, 88, 107, 72, 101, 61, 184,
                    132, 19, 110, 144, 107, 22, 178, 252, 206, 50, 207, 11, 177, 137, 35, 139, 68,
                    212, 148, 121, 249, 50, 35, 89, 52, 47, 26, 23, 6, 15, 115, 155, 127, 59, 168,
                    208, 196, 78, 125, 205, 0, 98, 43, 223, 233, 65, 137, 103, 2, 227, 35, 81, 107,
                    247, 230, 186, 111, 27, 4, 57, 42, 220, 32, 29, 181, 159, 6, 176, 182, 94, 191,
                    222, 212, 235, 60, 101, 83, 86, 217, 203, 151, 251, 254, 219, 204, 195, 10, 74,
                    147, 5, 27, 167, 127, 117, 149, 245, 157, 92, 124, 2, 196, 214, 107, 246, 228,
                    171, 229, 100, 212, 67, 88, 215, 75, 33, 183, 199, 51, 171, 210, 213, 65, 45,
                    96, 96, 226, 29, 130, 254, 58, 92, 252, 133, 207, 105, 63, 156, 208, 149, 142,
                    9, 83, 1, 193, 217, 244, 35, 137, 43, 138, 137, 140, 82, 231, 195, 145, 213,
                    230, 185, 245, 104, 105, 62, 142, 124, 34, 9, 157, 167, 188, 243, 112, 104,
                    248, 63, 50, 19, 53, 173, 69, 12, 39, 252, 9, 69, 223, 104, 99, 101, 114, 116,
                    73, 110, 102, 111, 88, 161, 255, 84, 67, 71, 128, 23, 0, 34, 0, 11, 174, 74,
                    152, 70, 1, 87, 191, 156, 96, 74, 177, 221, 37, 132, 6, 8, 101, 35, 124, 216,
                    85, 173, 85, 195, 115, 137, 194, 247, 145, 61, 82, 40, 0, 20, 234, 98, 144, 49,
                    146, 39, 99, 47, 44, 82, 115, 48, 64, 40, 152, 224, 227, 42, 63, 133, 0, 0, 0,
                    2, 219, 215, 137, 38, 187, 106, 183, 8, 100, 145, 106, 200, 1, 86, 5, 220, 81,
                    118, 234, 131, 141, 0, 34, 0, 11, 239, 53, 112, 255, 253, 12, 189, 168, 16,
                    253, 10, 149, 108, 7, 31, 212, 143, 21, 153, 7, 7, 153, 99, 73, 205, 97, 90,
                    110, 182, 120, 4, 250, 0, 34, 0, 11, 249, 72, 224, 84, 16, 96, 147, 197, 167,
                    195, 110, 181, 77, 207, 147, 16, 34, 64, 139, 185, 120, 190, 196, 209, 213, 29,
                    1, 136, 76, 235, 223, 247, 104, 97, 117, 116, 104, 68, 97, 116, 97, 89, 1, 103,
                    108, 41, 129, 232, 231, 178, 172, 146, 198, 102, 0, 255, 160, 250, 221, 227,
                    137, 40, 196, 142, 208, 221, 115, 246, 47, 198, 69, 45, 165, 107, 42, 27, 69,
                    0, 0, 0, 0, 8, 152, 112, 88, 202, 220, 75, 129, 182, 225, 48, 222, 80, 220,
                    190, 150, 0, 32, 211, 249, 248, 105, 48, 155, 162, 98, 212, 65, 122, 244, 237,
                    206, 196, 107, 226, 116, 136, 219, 221, 98, 101, 180, 109, 65, 174, 55, 175,
                    156, 120, 96, 164, 1, 3, 3, 57, 1, 0, 32, 89, 1, 0, 220, 20, 243, 114, 251,
                    142, 90, 236, 17, 204, 181, 223, 8, 72, 230, 209, 122, 44, 90, 55, 96, 134, 69,
                    16, 125, 139, 112, 81, 154, 230, 133, 211, 129, 37, 75, 208, 222, 70, 210, 239,
                    209, 188, 152, 93, 222, 222, 154, 169, 217, 160, 90, 243, 135, 151, 25, 87,
                    240, 178, 106, 119, 150, 89, 23, 223, 158, 88, 107, 72, 101, 61, 184, 132, 19,
                    110, 144, 107, 22, 178, 252, 206, 50, 207, 11, 177, 137, 35, 139, 68, 212, 148,
                    121, 249, 50, 35, 89, 52, 47, 26, 23, 6, 15, 115, 155, 127, 59, 168, 208, 196,
                    78, 125, 205, 0, 98, 43, 223, 233, 65, 137, 103, 2, 227, 35, 81, 107, 247, 230,
                    186, 111, 27, 4, 57, 42, 220, 32, 29, 181, 159, 6, 176, 182, 94, 191, 222, 212,
                    235, 60, 101, 83, 86, 217, 203, 151, 251, 254, 219, 204, 195, 10, 74, 147, 5,
                    27, 167, 127, 117, 149, 245, 157, 92, 124, 2, 196, 214, 107, 246, 228, 171,
                    229, 100, 212, 67, 88, 215, 75, 33, 183, 199, 51, 171, 210, 213, 65, 45, 96,
                    96, 226, 29, 130, 254, 58, 92, 252, 133, 207, 105, 63, 156, 208, 149, 142, 9,
                    83, 1, 193, 217, 244, 35, 137, 43, 138, 137, 140, 82, 231, 195, 145, 213, 230,
                    185, 245, 104, 105, 62, 142, 124, 34, 9, 157, 167, 188, 243, 112, 104, 248, 63,
                    50, 19, 53, 173, 69, 12, 39, 252, 9, 69, 223, 33, 67, 1, 0, 1,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 99, 114, 101, 97, 116, 101, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110,
                    103, 101, 34, 58, 34, 73, 108, 121, 57, 116, 68, 90, 99, 89, 76, 103, 66, 121,
                    74, 116, 98, 75, 113, 105, 99, 88, 118, 55, 102, 77, 97, 109, 114, 115, 119,
                    74, 72, 87, 110, 117, 48, 57, 67, 87, 50, 69, 84, 81, 34, 44, 34, 111, 114,
                    105, 103, 105, 110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 101, 116,
                    111, 111, 108, 115, 45, 100, 101, 118, 46, 101, 120, 97, 109, 112, 108, 101,
                    46, 99, 111, 109, 58, 56, 48, 56, 48, 34, 44, 34, 99, 114, 111, 115, 115, 79,
                    114, 105, 103, 105, 110, 34, 58, 102, 97, 108, 115, 101, 125,
                ]),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::RS256],
            Some(
                &(MICROSOFT_TPM_ROOT_CERTIFICATE_AUTHORITY_2014_PEM
                    .try_into()
                    .unwrap()),
            ),
            false,
            &RequestRegistrationExtensions::default(),
            true,
        );
        trace!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::CredentialInsecureCryptography)
        ))
    }

    fn register_userid(
        user_unique_id: &[u8],
        user_name: &str,
        user_display_name: &str,
    ) -> Result<(CreationChallengeResponse, RegistrationState), WebauthnError> {
        #![allow(clippy::unwrap_used)]

        let wan = Webauthn::new_unsafe_experts_only(
            "https://etools-dev.example.com:8080/auth",
            "etools-dev.example.com",
            vec![Url::parse("https://etools-dev.example.com:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let builder =
            wan.new_challenge_register_builder(user_unique_id, user_name, user_display_name)?;

        let builder = builder
            .user_verification_policy(UserVerificationPolicy::Required)
            .attestation(AttestationConveyancePreference::None)
            .exclude_credentials(None)
            .extensions(None)
            .credential_algorithms(COSEAlgorithm::secure_algs())
            .require_resident_key(false)
            .authenticator_attachment(None);

        wan.generate_challenge_register(builder)
    }

    #[test]
    fn test_registration_userid_states() {
        assert!(matches!(
            register_userid(&[], "an name", "an name"),
            Err(WebauthnError::InvalidUsername)
        ));
        assert!(matches!(
            register_userid(&[0, 1, 2, 3], "an name", ""),
            Err(WebauthnError::InvalidUsername)
        ));
        assert!(matches!(
            register_userid(&[0, 1, 2, 3], "", "an_name"),
            Err(WebauthnError::InvalidUsername)
        ));
        assert!(register_userid(&[0, 1, 2, 3], "fizzbuzz", "an name").is_ok());
    }

    #[test]
    fn test_touchid_attest_apple_anonymous() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://spectral.local:8443/auth",
            "spectral.local",
            vec![Url::parse("https://spectral.local:8443").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(vec![
            37, 54, 228, 239, 39, 164, 32, 163, 153, 67, 12, 29, 25, 110, 205, 120, 50, 31, 198,
            182, 10, 208, 251, 238, 99, 27, 46, 123, 239, 134, 244, 210,
        ]);

        let rsp_d = RegisterPublicKeyCredential {
            id: "u_tliFf-aXRLg9XIz-SuQ0XBlbE".to_string(),
            raw_id: Base64UrlSafeData::from(vec![
                187, 251, 101, 136, 87, 254, 105, 116, 75, 131, 213, 200, 207, 228, 174, 67, 69,
                193, 149, 177,
            ]),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(vec![
                    163, 99, 102, 109, 116, 101, 97, 112, 112, 108, 101, 103, 97, 116, 116, 83,
                    116, 109, 116, 162, 99, 97, 108, 103, 38, 99, 120, 53, 99, 130, 89, 2, 71, 48,
                    130, 2, 67, 48, 130, 1, 201, 160, 3, 2, 1, 2, 2, 6, 1, 118, 69, 82, 254, 167,
                    48, 10, 6, 8, 42, 134, 72, 206, 61, 4, 3, 2, 48, 72, 49, 28, 48, 26, 6, 3, 85,
                    4, 3, 12, 19, 65, 112, 112, 108, 101, 32, 87, 101, 98, 65, 117, 116, 104, 110,
                    32, 67, 65, 32, 49, 49, 19, 48, 17, 6, 3, 85, 4, 10, 12, 10, 65, 112, 112, 108,
                    101, 32, 73, 110, 99, 46, 49, 19, 48, 17, 6, 3, 85, 4, 8, 12, 10, 67, 97, 108,
                    105, 102, 111, 114, 110, 105, 97, 48, 30, 23, 13, 50, 48, 49, 50, 48, 56, 48,
                    50, 50, 55, 49, 53, 90, 23, 13, 50, 48, 49, 50, 49, 49, 48, 50, 50, 55, 49, 53,
                    90, 48, 129, 145, 49, 73, 48, 71, 6, 3, 85, 4, 3, 12, 64, 57, 97, 97, 57, 48,
                    99, 55, 99, 57, 51, 54, 97, 52, 101, 49, 98, 98, 56, 54, 56, 57, 54, 53, 102,
                    49, 52, 55, 97, 52, 51, 57, 57, 102, 49, 52, 48, 99, 102, 52, 48, 57, 98, 52,
                    51, 52, 102, 57, 48, 53, 57, 98, 50, 100, 52, 102, 53, 97, 51, 99, 102, 99, 48,
                    57, 50, 49, 26, 48, 24, 6, 3, 85, 4, 11, 12, 17, 65, 65, 65, 32, 67, 101, 114,
                    116, 105, 102, 105, 99, 97, 116, 105, 111, 110, 49, 19, 48, 17, 6, 3, 85, 4,
                    10, 12, 10, 65, 112, 112, 108, 101, 32, 73, 110, 99, 46, 49, 19, 48, 17, 6, 3,
                    85, 4, 8, 12, 10, 67, 97, 108, 105, 102, 111, 114, 110, 105, 97, 48, 89, 48,
                    19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 3,
                    66, 0, 4, 212, 248, 99, 135, 245, 78, 94, 245, 231, 22, 62, 226, 45, 40, 215,
                    4, 251, 188, 180, 125, 22, 236, 133, 161, 234, 78, 251, 105, 11, 119, 148, 144,
                    105, 249, 199, 167, 152, 173, 94, 147, 57, 2, 250, 21, 5, 51, 116, 174, 217,
                    39, 160, 35, 12, 249, 120, 237, 52, 148, 171, 134, 138, 205, 26, 173, 163, 85,
                    48, 83, 48, 12, 6, 3, 85, 29, 19, 1, 1, 255, 4, 2, 48, 0, 48, 14, 6, 3, 85, 29,
                    15, 1, 1, 255, 4, 4, 3, 2, 4, 240, 48, 51, 6, 9, 42, 134, 72, 134, 247, 99,
                    100, 8, 2, 4, 38, 48, 36, 161, 34, 4, 32, 168, 226, 160, 197, 61, 146, 15, 234,
                    100, 124, 22, 29, 34, 18, 171, 91, 253, 122, 81, 241, 182, 105, 240, 209, 130,
                    176, 179, 61, 84, 183, 78, 190, 48, 10, 6, 8, 42, 134, 72, 206, 61, 4, 3, 2, 3,
                    104, 0, 48, 101, 2, 48, 14, 242, 134, 73, 12, 48, 2, 103, 184, 132, 187, 132,
                    124, 204, 63, 148, 168, 78, 225, 227, 161, 240, 147, 187, 90, 216, 65, 159, 90,
                    106, 102, 249, 56, 156, 201, 214, 182, 15, 173, 187, 167, 243, 127, 234, 138,
                    41, 50, 62, 2, 49, 0, 198, 15, 10, 182, 142, 103, 84, 7, 18, 0, 231, 130, 214,
                    26, 64, 58, 17, 118, 66, 14, 198, 244, 58, 211, 2, 97, 236, 163, 116, 124, 73,
                    166, 69, 69, 112, 107, 228, 83, 104, 91, 205, 20, 203, 250, 126, 29, 190, 42,
                    89, 2, 56, 48, 130, 2, 52, 48, 130, 1, 186, 160, 3, 2, 1, 2, 2, 16, 86, 37, 83,
                    149, 199, 167, 251, 64, 235, 226, 40, 216, 38, 8, 83, 182, 48, 10, 6, 8, 42,
                    134, 72, 206, 61, 4, 3, 3, 48, 75, 49, 31, 48, 29, 6, 3, 85, 4, 3, 12, 22, 65,
                    112, 112, 108, 101, 32, 87, 101, 98, 65, 117, 116, 104, 110, 32, 82, 111, 111,
                    116, 32, 67, 65, 49, 19, 48, 17, 6, 3, 85, 4, 10, 12, 10, 65, 112, 112, 108,
                    101, 32, 73, 110, 99, 46, 49, 19, 48, 17, 6, 3, 85, 4, 8, 12, 10, 67, 97, 108,
                    105, 102, 111, 114, 110, 105, 97, 48, 30, 23, 13, 50, 48, 48, 51, 49, 56, 49,
                    56, 51, 56, 48, 49, 90, 23, 13, 51, 48, 48, 51, 49, 51, 48, 48, 48, 48, 48, 48,
                    90, 48, 72, 49, 28, 48, 26, 6, 3, 85, 4, 3, 12, 19, 65, 112, 112, 108, 101, 32,
                    87, 101, 98, 65, 117, 116, 104, 110, 32, 67, 65, 32, 49, 49, 19, 48, 17, 6, 3,
                    85, 4, 10, 12, 10, 65, 112, 112, 108, 101, 32, 73, 110, 99, 46, 49, 19, 48, 17,
                    6, 3, 85, 4, 8, 12, 10, 67, 97, 108, 105, 102, 111, 114, 110, 105, 97, 48, 118,
                    48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 5, 43, 129, 4, 0, 34, 3, 98, 0, 4,
                    131, 46, 135, 47, 38, 20, 145, 129, 2, 37, 185, 245, 252, 214, 187, 99, 120,
                    181, 245, 95, 63, 203, 4, 91, 199, 53, 153, 52, 117, 253, 84, 144, 68, 223,
                    155, 254, 25, 33, 23, 101, 198, 154, 29, 218, 5, 11, 56, 212, 80, 131, 64, 26,
                    67, 79, 178, 77, 17, 45, 86, 195, 225, 207, 191, 203, 152, 145, 254, 192, 105,
                    96, 129, 190, 249, 108, 188, 119, 200, 141, 221, 175, 70, 165, 174, 225, 221,
                    81, 91, 90, 250, 171, 147, 190, 156, 11, 38, 145, 163, 102, 48, 100, 48, 18, 6,
                    3, 85, 29, 19, 1, 1, 255, 4, 8, 48, 6, 1, 1, 255, 2, 1, 0, 48, 31, 6, 3, 85,
                    29, 35, 4, 24, 48, 22, 128, 20, 38, 215, 100, 217, 197, 120, 194, 90, 103, 209,
                    167, 222, 107, 18, 208, 27, 99, 241, 198, 215, 48, 29, 6, 3, 85, 29, 14, 4, 22,
                    4, 20, 235, 174, 130, 196, 255, 161, 172, 91, 81, 212, 207, 36, 97, 5, 0, 190,
                    99, 189, 119, 136, 48, 14, 6, 3, 85, 29, 15, 1, 1, 255, 4, 4, 3, 2, 1, 6, 48,
                    10, 6, 8, 42, 134, 72, 206, 61, 4, 3, 3, 3, 104, 0, 48, 101, 2, 49, 0, 221,
                    139, 26, 52, 129, 165, 250, 217, 219, 180, 231, 101, 123, 132, 30, 20, 76, 39,
                    183, 91, 135, 106, 65, 134, 194, 177, 71, 87, 80, 51, 114, 39, 239, 229, 84,
                    69, 126, 246, 72, 149, 12, 99, 46, 92, 72, 62, 112, 193, 2, 48, 44, 138, 96,
                    68, 220, 32, 31, 207, 229, 155, 195, 77, 41, 48, 193, 72, 120, 81, 217, 96,
                    237, 106, 117, 241, 235, 74, 202, 190, 56, 205, 37, 184, 151, 208, 200, 5, 190,
                    240, 199, 247, 139, 7, 165, 113, 198, 232, 14, 7, 104, 97, 117, 116, 104, 68,
                    97, 116, 97, 88, 152, 218, 20, 177, 242, 169, 30, 45, 223, 21, 45, 254, 74, 34,
                    125, 188, 96, 11, 1, 71, 41, 58, 94, 252, 180, 169, 243, 209, 21, 231, 138,
                    182, 91, 69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20,
                    187, 251, 101, 136, 87, 254, 105, 116, 75, 131, 213, 200, 207, 228, 174, 67,
                    69, 193, 149, 177, 165, 1, 2, 3, 38, 32, 1, 33, 88, 32, 212, 248, 99, 135, 245,
                    78, 94, 245, 231, 22, 62, 226, 45, 40, 215, 4, 251, 188, 180, 125, 22, 236,
                    133, 161, 234, 78, 251, 105, 11, 119, 148, 144, 34, 88, 32, 105, 249, 199, 167,
                    152, 173, 94, 147, 57, 2, 250, 21, 5, 51, 116, 174, 217, 39, 160, 35, 12, 249,
                    120, 237, 52, 148, 171, 134, 138, 205, 26, 173,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 99, 114, 101, 97, 116, 101, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110,
                    103, 101, 34, 58, 34, 74, 84, 98, 107, 55, 121, 101, 107, 73, 75, 79, 90, 81,
                    119, 119, 100, 71, 87, 55, 78, 101, 68, 73, 102, 120, 114, 89, 75, 48, 80, 118,
                    117, 89, 120, 115, 117, 101, 45, 45, 71, 57, 78, 73, 34, 44, 34, 111, 114, 105,
                    103, 105, 110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 115, 112, 101,
                    99, 116, 114, 97, 108, 46, 108, 111, 99, 97, 108, 58, 56, 52, 52, 51, 34, 125,
                ]),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        // Attempt to request an AAGUID, but this format does not provide one.
        let mut att_ca_builder = AttestationCaListBuilder::new();
        att_ca_builder
            .insert_device_pem(
                APPLE_WEBAUTHN_ROOT_CA_PEM,
                uuid::uuid!("c5ef55ff-ad9a-4b9f-b580-adebafe026d0"),
                "yk 5 ci".to_string(),
                Default::default(),
            )
            .expect("Failed to build att ca list");
        let att_ca_list: AttestationCaList = att_ca_builder.build();

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[
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
            ],
            Some(&att_ca_list),
            // Must disable time checks because the submission is limited to 5 days.
            true,
            &RequestRegistrationExtensions::default(),
            // Don't allow passkeys
            false,
        );
        debug!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::AttestationFormatMissingAaguid)
        ));

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[
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
            ],
            Some(&(APPLE_WEBAUTHN_ROOT_CA_PEM.try_into().unwrap())),
            // Must disable time checks because the submission is limited to 5 days.
            true,
            &RequestRegistrationExtensions::default(),
            // Don't allow passkeys
            false,
        );
        debug!("{:?}", result);
        assert!(result.is_ok());

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[
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
            ],
            Some(&(APPLE_WEBAUTHN_ROOT_CA_PEM.try_into().unwrap())),
            // Must disable time checks because the submission is limited to 5 days.
            true,
            &RequestRegistrationExtensions::default(),
            // Allow them.
            true,
        );
        debug!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_touchid_attest_apple_anonymous_fails_with_invalid_nonce_extension() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://spectral.local:8443/auth",
            "spectral.local",
            vec![Url::parse("https://spectral.local:8443").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::new(vec![
            37, 54, 228, 239, 39, 164, 32, 163, 153, 67, 12, 29, 25, 110, 205, 120, 50, 31, 198,
            182, 10, 208, 251, 238, 99, 27, 46, 123, 239, 134, 244, 210,
        ]);

        let rsp_d = RegisterPublicKeyCredential {
            id: "u_tliFf-aXRLg9XIz-SuQ0XBlbE".to_string(),
            raw_id: Base64UrlSafeData::from(vec![
                187, 251, 101, 136, 87, 254, 105, 116, 75, 131, 213, 200, 207, 228, 174, 67, 69,
                193, 149, 177,
            ]),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(vec![
                    163, 99, 102, 109, 116, 101, 97, 112, 112, 108, 101, 103, 97, 116, 116, 83,
                    116, 109, 116, 162, 99, 97, 108, 103, 38, 99, 120, 53, 99, 130, 89, 2, 71, 48,
                    130, 2, 67, 48, 130, 1, 201, 160, 3, 2, 1, 2, 2, 6, 1, 118, 69, 82, 254, 167,
                    48, 10, 6, 8, 42, 134, 72, 206, 61, 4, 3, 2, 48, 72, 49, 28, 48, 26, 6, 3, 85,
                    4, 3, 12, 19, 65, 112, 112, 108, 101, 32, 87, 101, 98, 65, 117, 116, 104, 110,
                    32, 67, 65, 32, 49, 49, 19, 48, 17, 6, 3, 85, 4, 10, 12, 10, 65, 112, 112, 108,
                    101, 32, 73, 110, 99, 46, 49, 19, 48, 17, 6, 3, 85, 4, 8, 12, 10, 67, 97, 108,
                    105, 102, 111, 114, 110, 105, 97, 48, 30, 23, 13, 50, 48, 49, 50, 48, 56, 48,
                    50, 50, 55, 49, 53, 90, 23, 13, 50, 48, 49, 50, 49, 49, 48, 50, 50, 55, 49, 53,
                    90, 48, 129, 145, 49, 73, 48, 71, 6, 3, 85, 4, 3, 12, 64, 57, 97, 97, 57, 48,
                    99, 55, 99, 57, 51, 54, 97, 52, 101, 49, 98, 98, 56, 54, 56, 57, 54, 53, 102,
                    49, 52, 55, 97, 52, 51, 57, 57, 102, 49, 52, 48, 99, 102, 52, 48, 57, 98, 52,
                    51, 52, 102, 57, 48, 53, 57, 98, 50, 100, 52, 102, 53, 97, 51, 99, 102, 99, 48,
                    57, 50, 49, 26, 48, 24, 6, 3, 85, 4, 11, 12, 17, 65, 65, 65, 32, 67, 101, 114,
                    116, 105, 102, 105, 99, 97, 116, 105, 111, 110, 49, 19, 48, 17, 6, 3, 85, 4,
                    10, 12, 10, 65, 112, 112, 108, 101, 32, 73, 110, 99, 46, 49, 19, 48, 17, 6, 3,
                    85, 4, 8, 12, 10, 67, 97, 108, 105, 102, 111, 114, 110, 105, 97, 48, 89, 48,
                    19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1, 7, 3,
                    66, 0, 4, 212, 248, 99, 135, 245, 78, 94, 245, 231, 22, 62, 226, 45, 40, 215,
                    4, 251, 188, 180, 125, 22, 236, 133, 161, 234, 78, 251, 105, 11, 119, 148, 144,
                    105, 249, 199, 167, 152, 173, 94, 147, 57, 2, 250, 21, 5, 51, 116, 174, 217,
                    39, 160, 35, 12, 249, 120, 237, 52, 148, 171, 134, 138, 205, 26, 173, 163, 85,
                    48, 83, 48, 12, 6, 3, 85, 29, 19, 1, 1, 255, 4, 2, 48, 0, 48, 14, 6, 3, 85, 29,
                    15, 1, 1, 255, 4, 4, 3, 2, 4, 240, 48, 51, 6, 9, 42, 134, 72, 134, 247, 99,
                    100, 8, 2, 4, 38, 48, 36, 161, 34, 4, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 48, 10, 6, 8, 42,
                    134, 72, 206, 61, 4, 3, 2, 3, 104, 0, 48, 101, 2, 48, 14, 242, 134, 73, 12, 48,
                    2, 103, 184, 132, 187, 132, 124, 204, 63, 148, 168, 78, 225, 227, 161, 240,
                    147, 187, 90, 216, 65, 159, 90, 106, 102, 249, 56, 156, 201, 214, 182, 15, 173,
                    187, 167, 243, 127, 234, 138, 41, 50, 62, 2, 49, 0, 198, 15, 10, 182, 142, 103,
                    84, 7, 18, 0, 231, 130, 214, 26, 64, 58, 17, 118, 66, 14, 198, 244, 58, 211, 2,
                    97, 236, 163, 116, 124, 73, 166, 69, 69, 112, 107, 228, 83, 104, 91, 205, 20,
                    203, 250, 126, 29, 190, 42, 89, 2, 56, 48, 130, 2, 52, 48, 130, 1, 186, 160, 3,
                    2, 1, 2, 2, 16, 86, 37, 83, 149, 199, 167, 251, 64, 235, 226, 40, 216, 38, 8,
                    83, 182, 48, 10, 6, 8, 42, 134, 72, 206, 61, 4, 3, 3, 48, 75, 49, 31, 48, 29,
                    6, 3, 85, 4, 3, 12, 22, 65, 112, 112, 108, 101, 32, 87, 101, 98, 65, 117, 116,
                    104, 110, 32, 82, 111, 111, 116, 32, 67, 65, 49, 19, 48, 17, 6, 3, 85, 4, 10,
                    12, 10, 65, 112, 112, 108, 101, 32, 73, 110, 99, 46, 49, 19, 48, 17, 6, 3, 85,
                    4, 8, 12, 10, 67, 97, 108, 105, 102, 111, 114, 110, 105, 97, 48, 30, 23, 13,
                    50, 48, 48, 51, 49, 56, 49, 56, 51, 56, 48, 49, 90, 23, 13, 51, 48, 48, 51, 49,
                    51, 48, 48, 48, 48, 48, 48, 90, 48, 72, 49, 28, 48, 26, 6, 3, 85, 4, 3, 12, 19,
                    65, 112, 112, 108, 101, 32, 87, 101, 98, 65, 117, 116, 104, 110, 32, 67, 65,
                    32, 49, 49, 19, 48, 17, 6, 3, 85, 4, 10, 12, 10, 65, 112, 112, 108, 101, 32,
                    73, 110, 99, 46, 49, 19, 48, 17, 6, 3, 85, 4, 8, 12, 10, 67, 97, 108, 105, 102,
                    111, 114, 110, 105, 97, 48, 118, 48, 16, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6,
                    5, 43, 129, 4, 0, 34, 3, 98, 0, 4, 131, 46, 135, 47, 38, 20, 145, 129, 2, 37,
                    185, 245, 252, 214, 187, 99, 120, 181, 245, 95, 63, 203, 4, 91, 199, 53, 153,
                    52, 117, 253, 84, 144, 68, 223, 155, 254, 25, 33, 23, 101, 198, 154, 29, 218,
                    5, 11, 56, 212, 80, 131, 64, 26, 67, 79, 178, 77, 17, 45, 86, 195, 225, 207,
                    191, 203, 152, 145, 254, 192, 105, 96, 129, 190, 249, 108, 188, 119, 200, 141,
                    221, 175, 70, 165, 174, 225, 221, 81, 91, 90, 250, 171, 147, 190, 156, 11, 38,
                    145, 163, 102, 48, 100, 48, 18, 6, 3, 85, 29, 19, 1, 1, 255, 4, 8, 48, 6, 1, 1,
                    255, 2, 1, 0, 48, 31, 6, 3, 85, 29, 35, 4, 24, 48, 22, 128, 20, 38, 215, 100,
                    217, 197, 120, 194, 90, 103, 209, 167, 222, 107, 18, 208, 27, 99, 241, 198,
                    215, 48, 29, 6, 3, 85, 29, 14, 4, 22, 4, 20, 235, 174, 130, 196, 255, 161, 172,
                    91, 81, 212, 207, 36, 97, 5, 0, 190, 99, 189, 119, 136, 48, 14, 6, 3, 85, 29,
                    15, 1, 1, 255, 4, 4, 3, 2, 1, 6, 48, 10, 6, 8, 42, 134, 72, 206, 61, 4, 3, 3,
                    3, 104, 0, 48, 101, 2, 49, 0, 221, 139, 26, 52, 129, 165, 250, 217, 219, 180,
                    231, 101, 123, 132, 30, 20, 76, 39, 183, 91, 135, 106, 65, 134, 194, 177, 71,
                    87, 80, 51, 114, 39, 239, 229, 84, 69, 126, 246, 72, 149, 12, 99, 46, 92, 72,
                    62, 112, 193, 2, 48, 44, 138, 96, 68, 220, 32, 31, 207, 229, 155, 195, 77, 41,
                    48, 193, 72, 120, 81, 217, 96, 237, 106, 117, 241, 235, 74, 202, 190, 56, 205,
                    37, 184, 151, 208, 200, 5, 190, 240, 199, 247, 139, 7, 165, 113, 198, 232, 14,
                    7, 104, 97, 117, 116, 104, 68, 97, 116, 97, 88, 152, 218, 20, 177, 242, 169,
                    30, 45, 223, 21, 45, 254, 74, 34, 125, 188, 96, 11, 1, 71, 41, 58, 94, 252,
                    180, 169, 243, 209, 21, 231, 138, 182, 91, 69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 187, 251, 101, 136, 87, 254, 105, 116, 75,
                    131, 213, 200, 207, 228, 174, 67, 69, 193, 149, 177, 165, 1, 2, 3, 38, 32, 1,
                    33, 88, 32, 212, 248, 99, 135, 245, 78, 94, 245, 231, 22, 62, 226, 45, 40, 215,
                    4, 251, 188, 180, 125, 22, 236, 133, 161, 234, 78, 251, 105, 11, 119, 148, 144,
                    34, 88, 32, 105, 249, 199, 167, 152, 173, 94, 147, 57, 2, 250, 21, 5, 51, 116,
                    174, 217, 39, 160, 35, 12, 249, 120, 237, 52, 148, 171, 134, 138, 205, 26, 173,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 99, 114, 101, 97, 116, 101, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110,
                    103, 101, 34, 58, 34, 74, 84, 98, 107, 55, 121, 101, 107, 73, 75, 79, 90, 81,
                    119, 119, 100, 71, 87, 55, 78, 101, 68, 73, 102, 120, 114, 89, 75, 48, 80, 118,
                    117, 89, 120, 115, 117, 101, 45, 45, 71, 57, 78, 73, 34, 44, 34, 111, 114, 105,
                    103, 105, 110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 115, 112, 101,
                    99, 116, 114, 97, 108, 46, 108, 111, 99, 97, 108, 58, 56, 52, 52, 51, 34, 125,
                ]),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[
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
            ],
            Some(&(APPLE_WEBAUTHN_ROOT_CA_PEM.try_into().unwrap())),
            // Must disable time checks because the submission is limited to 5 days.
            true,
            &RequestRegistrationExtensions::default(),
            false,
        );
        debug!("{:?}", result);
        assert!(matches!(
            result,
            Err(WebauthnError::AttestationCertificateNonceMismatch)
        ));
    }

    #[test]
    fn test_uv_consistency() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "http://127.0.0.1:8080/auth",
            "127.0.0.1",
            vec![Url::parse("http://127.0.0.1:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        // Given two credentials with differening policy
        let mut creds = vec![
            Credential {
                cred_id: HumanBinaryData::from(vec![
                    205, 198, 18, 130, 133, 220, 73, 23, 199, 211, 240, 143, 220, 154, 172, 117,
                    91, 18, 164, 211, 24, 147, 16, 203, 118, 76, 33, 65, 31, 92, 236, 211, 79, 67,
                    240, 30, 65, 247, 46, 134, 19, 136, 170, 209, 11, 115, 37, 12, 88, 244, 244,
                    240, 148, 132, 191, 165, 150, 166, 252, 39, 97, 137, 21, 186,
                ]),
                cred: COSEKey {
                    type_: COSEAlgorithm::ES256,
                    key: COSEKeyType::EC_EC2(COSEEC2Key {
                        curve: ECDSACurve::SECP256R1,
                        x: [
                            131, 160, 173, 103, 102, 41, 186, 183, 60, 175, 136, 103, 167, 145,
                            239, 235, 216, 80, 109, 26, 218, 187, 146, 77, 5, 173, 143, 33, 126,
                            119, 197, 116,
                        ]
                        .to_vec()
                        .into(),
                        y: [
                            59, 202, 240, 192, 92, 25, 186, 100, 135, 111, 53, 194, 234, 134, 249,
                            249, 30, 22, 70, 58, 81, 250, 141, 38, 217, 9, 44, 121, 162, 230, 197,
                            87,
                        ]
                        .to_vec()
                        .into(),
                    }),
                },
                counter: 0,
                transports: None,
                user_verified: false,
                backup_eligible: false,
                backup_state: false,
                registration_policy: UserVerificationPolicy::Discouraged_DO_NOT_USE,
                extensions: RegisteredExtensions::none(),
                attestation: ParsedAttestation {
                    data: ParsedAttestationData::None,
                    metadata: AttestationMetadata::None,
                },
                attestation_format: AttestationFormat::None,
            },
            Credential {
                cred_id: HumanBinaryData::from(vec![
                    211, 204, 163, 253, 101, 149, 83, 136, 242, 175, 211, 104, 215, 131, 122, 175,
                    187, 84, 13, 3, 21, 24, 11, 138, 50, 137, 55, 225, 180, 109, 49, 28, 98, 8, 28,
                    181, 149, 241, 106, 124, 110, 149, 154, 198, 23, 8, 8, 4, 41, 69, 236, 203,
                    122, 120, 204, 174, 28, 58, 171, 43, 218, 81, 195, 177,
                ]),
                cred: COSEKey {
                    type_: COSEAlgorithm::ES256,
                    key: COSEKeyType::EC_EC2(COSEEC2Key {
                        curve: ECDSACurve::SECP256R1,
                        x: [
                            87, 236, 127, 24, 222, 164, 79, 139, 67, 77, 159, 33, 76, 155, 161,
                            155, 234, 151, 203, 142, 136, 87, 77, 177, 27, 67, 248, 104, 233, 156,
                            15, 51,
                        ]
                        .to_vec()
                        .into(),
                        y: [
                            21, 29, 94, 187, 68, 148, 156, 253, 117, 226, 40, 88, 53, 61, 209, 227,
                            12, 164, 136, 185, 148, 125, 86, 21, 22, 52, 195, 192, 6, 6, 176, 179,
                        ]
                        .to_vec()
                        .into(),
                    }),
                },
                counter: 1,
                transports: None,
                user_verified: true,
                backup_eligible: false,
                backup_state: false,
                registration_policy: UserVerificationPolicy::Required,
                extensions: RegisteredExtensions::none(),
                attestation: ParsedAttestation {
                    data: ParsedAttestationData::None,
                    metadata: AttestationMetadata::None,
                },
                attestation_format: AttestationFormat::None,
            },
        ];
        // Ensure we get a bad result.

        assert!(
            wan.new_challenge_authenticate_builder(creds.clone(), None)
                .unwrap_err()
                == WebauthnError::InconsistentUserVerificationPolicy
        );

        // now mutate to different states to check.
        // cred 0 verified + uv::req
        // cred 1 verified + uv::req
        {
            creds
                .get_mut(0)
                .map(|cred| {
                    cred.user_verified = true;
                    cred.registration_policy = UserVerificationPolicy::Required;
                })
                .unwrap();
            creds
                .get_mut(1)
                .map(|cred| {
                    cred.user_verified = true;
                    cred.registration_policy = UserVerificationPolicy::Required;
                })
                .unwrap();
        }

        let builder = wan
            .new_challenge_authenticate_builder(creds.clone(), None)
            .expect("Unable to create authenticate builder");

        let r = wan.generate_challenge_authenticate(builder);
        debug!("{:?}", r);
        assert!(r.is_ok());

        // now mutate to different states to check.
        // cred 0 verified + uv::dc
        // cred 1 verified + uv::dc
        {
            creds
                .get_mut(0)
                .map(|cred| {
                    cred.user_verified = true;
                    cred.registration_policy = UserVerificationPolicy::Discouraged_DO_NOT_USE;
                })
                .unwrap();
            creds
                .get_mut(1)
                .map(|cred| {
                    cred.user_verified = false;
                    cred.registration_policy = UserVerificationPolicy::Discouraged_DO_NOT_USE;
                })
                .unwrap();
        }

        let builder = wan
            .new_challenge_authenticate_builder(creds.clone(), None)
            .expect("Unable to create authenticate builder");

        let r = wan.generate_challenge_authenticate(builder);

        debug!("{:?}", r);
        assert!(r.is_ok());
    }

    #[test]
    fn test_subdomain_origin() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "rp_name",
            "idm.example.com",
            vec![Url::parse("https://idm.example.com:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            Some(true),
            None,
        );

        let id =
            "zIQDbMsgDg89LbWHAMLrpgI4w5Bz5Hy8U6F-gaUmda1fgwgn6NzhXQFJwEDfowsiY0NTgdU2jjAG2PmzaD5aWA".to_string();
        let raw_id = Base64UrlSafeData::from(vec![
            204, 132, 3, 108, 203, 32, 14, 15, 61, 45, 181, 135, 0, 194, 235, 166, 2, 56, 195, 144,
            115, 228, 124, 188, 83, 161, 126, 129, 165, 38, 117, 173, 95, 131, 8, 39, 232, 220,
            225, 93, 1, 73, 192, 64, 223, 163, 11, 34, 99, 67, 83, 129, 213, 54, 142, 48, 6, 216,
            249, 179, 104, 62, 90, 88,
        ]);

        let chal = Challenge::new(vec![
            174, 237, 157, 66, 159, 70, 216, 148, 130, 184, 54, 89, 38, 149, 217, 32, 161, 42, 99,
            227, 50, 124, 208, 164, 221, 38, 202, 210, 140, 102, 116, 84,
        ]);

        let rsp_d = RegisterPublicKeyCredential {
            id: id.clone(),
            raw_id: raw_id.clone(),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(vec![
                    163, 99, 102, 109, 116, 104, 102, 105, 100, 111, 45, 117, 50, 102, 103, 97,
                    116, 116, 83, 116, 109, 116, 162, 99, 115, 105, 103, 88, 70, 48, 68, 2, 32,
                    125, 195, 114, 22, 37, 221, 215, 19, 15, 177, 53, 167, 63, 179, 235, 152, 8,
                    204, 65, 203, 37, 196, 223, 76, 226, 35, 234, 182, 102, 156, 93, 50, 2, 32, 20,
                    177, 103, 196, 47, 107, 19, 76, 35, 2, 14, 186, 197, 229, 113, 38, 83, 252, 17,
                    164, 221, 19, 27, 34, 193, 155, 205, 220, 133, 53, 47, 223, 99, 120, 53, 99,
                    129, 89, 2, 193, 48, 130, 2, 189, 48, 130, 1, 165, 160, 3, 2, 1, 2, 2, 4, 24,
                    172, 70, 192, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 11, 5, 0, 48, 46,
                    49, 44, 48, 42, 6, 3, 85, 4, 3, 19, 35, 89, 117, 98, 105, 99, 111, 32, 85, 50,
                    70, 32, 82, 111, 111, 116, 32, 67, 65, 32, 83, 101, 114, 105, 97, 108, 32, 52,
                    53, 55, 50, 48, 48, 54, 51, 49, 48, 32, 23, 13, 49, 52, 48, 56, 48, 49, 48, 48,
                    48, 48, 48, 48, 90, 24, 15, 50, 48, 53, 48, 48, 57, 48, 52, 48, 48, 48, 48, 48,
                    48, 90, 48, 110, 49, 11, 48, 9, 6, 3, 85, 4, 6, 19, 2, 83, 69, 49, 18, 48, 16,
                    6, 3, 85, 4, 10, 12, 9, 89, 117, 98, 105, 99, 111, 32, 65, 66, 49, 34, 48, 32,
                    6, 3, 85, 4, 11, 12, 25, 65, 117, 116, 104, 101, 110, 116, 105, 99, 97, 116,
                    111, 114, 32, 65, 116, 116, 101, 115, 116, 97, 116, 105, 111, 110, 49, 39, 48,
                    37, 6, 3, 85, 4, 3, 12, 30, 89, 117, 98, 105, 99, 111, 32, 85, 50, 70, 32, 69,
                    69, 32, 83, 101, 114, 105, 97, 108, 32, 52, 49, 51, 57, 52, 51, 52, 56, 56, 48,
                    89, 48, 19, 6, 7, 42, 134, 72, 206, 61, 2, 1, 6, 8, 42, 134, 72, 206, 61, 3, 1,
                    7, 3, 66, 0, 4, 121, 234, 59, 44, 124, 73, 112, 16, 98, 35, 12, 210, 63, 235,
                    96, 229, 41, 49, 113, 212, 131, 241, 0, 190, 133, 157, 107, 15, 131, 151, 3, 1,
                    181, 70, 205, 212, 110, 207, 202, 227, 227, 243, 15, 129, 233, 237, 98, 189,
                    38, 141, 76, 30, 189, 55, 179, 188, 190, 146, 168, 194, 174, 235, 78, 58, 163,
                    108, 48, 106, 48, 34, 6, 9, 43, 6, 1, 4, 1, 130, 196, 10, 2, 4, 21, 49, 46, 51,
                    46, 54, 46, 49, 46, 52, 46, 49, 46, 52, 49, 52, 56, 50, 46, 49, 46, 55, 48, 19,
                    6, 11, 43, 6, 1, 4, 1, 130, 229, 28, 2, 1, 1, 4, 4, 3, 2, 5, 32, 48, 33, 6, 11,
                    43, 6, 1, 4, 1, 130, 229, 28, 1, 1, 4, 4, 18, 4, 16, 203, 105, 72, 30, 143,
                    247, 64, 57, 147, 236, 10, 39, 41, 161, 84, 168, 48, 12, 6, 3, 85, 29, 19, 1,
                    1, 255, 4, 2, 48, 0, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13, 1, 1, 11, 5, 0,
                    3, 130, 1, 1, 0, 151, 157, 3, 151, 216, 96, 248, 46, 225, 93, 49, 28, 121, 110,
                    186, 251, 34, 250, 167, 224, 132, 217, 186, 180, 198, 27, 187, 87, 243, 230,
                    180, 193, 138, 72, 55, 184, 92, 60, 78, 219, 228, 131, 67, 244, 214, 165, 217,
                    177, 206, 218, 138, 225, 254, 212, 145, 41, 33, 115, 5, 142, 94, 225, 203, 221,
                    107, 218, 192, 117, 87, 198, 160, 232, 211, 104, 37, 186, 21, 158, 127, 181,
                    173, 140, 218, 248, 4, 134, 140, 249, 14, 143, 31, 138, 234, 23, 192, 22, 181,
                    92, 42, 122, 212, 151, 200, 148, 251, 113, 215, 83, 215, 155, 154, 72, 75, 108,
                    55, 109, 114, 59, 153, 141, 46, 29, 67, 6, 191, 16, 51, 181, 174, 248, 204,
                    165, 203, 178, 86, 139, 105, 36, 34, 109, 34, 163, 88, 171, 125, 135, 228, 172,
                    95, 46, 9, 26, 167, 21, 121, 243, 165, 105, 9, 73, 125, 114, 245, 78, 6, 186,
                    193, 195, 180, 65, 59, 186, 94, 175, 148, 195, 182, 79, 52, 249, 235, 164, 26,
                    203, 106, 226, 131, 119, 109, 54, 70, 83, 120, 72, 254, 232, 132, 189, 221,
                    245, 177, 186, 87, 152, 84, 207, 253, 206, 186, 195, 68, 5, 149, 39, 229, 109,
                    213, 152, 248, 245, 102, 113, 90, 190, 67, 1, 221, 25, 17, 48, 230, 185, 240,
                    198, 64, 57, 18, 83, 226, 41, 128, 63, 58, 239, 39, 75, 237, 191, 222, 63, 203,
                    189, 66, 234, 214, 121, 104, 97, 117, 116, 104, 68, 97, 116, 97, 88, 196, 239,
                    115, 241, 111, 91, 226, 27, 23, 185, 145, 15, 75, 208, 190, 109, 73, 186, 119,
                    107, 122, 2, 224, 117, 140, 139, 132, 92, 21, 148, 105, 187, 55, 65, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 204, 132, 3, 108,
                    203, 32, 14, 15, 61, 45, 181, 135, 0, 194, 235, 166, 2, 56, 195, 144, 115, 228,
                    124, 188, 83, 161, 126, 129, 165, 38, 117, 173, 95, 131, 8, 39, 232, 220, 225,
                    93, 1, 73, 192, 64, 223, 163, 11, 34, 99, 67, 83, 129, 213, 54, 142, 48, 6,
                    216, 249, 179, 104, 62, 90, 88, 165, 1, 2, 3, 38, 32, 1, 33, 88, 32, 169, 47,
                    103, 25, 132, 175, 84, 4, 152, 225, 66, 5, 83, 201, 162, 184, 13, 204, 129,
                    162, 225, 184, 248, 76, 21, 9, 140, 51, 233, 28, 21, 189, 34, 88, 32, 152, 216,
                    30, 49, 240, 214, 59, 66, 44, 67, 110, 41, 126, 83, 131, 50, 13, 175, 237, 57,
                    225, 87, 38, 132, 17, 54, 52, 22, 0, 142, 54, 255,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 99, 114, 101, 97, 116, 101, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110,
                    103, 101, 34, 58, 34, 114, 117, 50, 100, 81, 112, 57, 71, 50, 74, 83, 67, 117,
                    68, 90, 90, 74, 112, 88, 90, 73, 75, 69, 113, 89, 45, 77, 121, 102, 78, 67,
                    107, 51, 83, 98, 75, 48, 111, 120, 109, 100, 70, 81, 34, 44, 34, 111, 114, 105,
                    103, 105, 110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 105, 100, 109,
                    46, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109, 58, 56, 48, 56, 48, 34,
                    44, 34, 99, 114, 111, 115, 115, 79, 114, 105, 103, 105, 110, 34, 58, 102, 97,
                    108, 115, 101, 125,
                ]),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        let cred = wan
            .register_credential_internal(
                &rsp_d,
                UserVerificationPolicy::Discouraged_DO_NOT_USE,
                &chal,
                &[],
                &[COSEAlgorithm::ES256],
                None,
                false,
                &RequestRegistrationExtensions::default(),
                true,
            )
            .expect("Failed to register credential");

        // In this we visit from "https://sub.idm.example.com:8080" which is an effective domain
        // of the origin.

        let chal = Challenge::new(vec![
            127, 52, 208, 243, 214, 88, 79, 34, 12, 226, 145, 217, 217, 241, 99, 228, 171, 232,
            226, 26, 191, 32, 122, 4, 164, 217, 49, 134, 85, 161, 116, 32,
        ]);

        let rsp_d = PublicKeyCredential {
            id,
            raw_id,
            response: AuthenticatorAssertionResponseRaw {
                authenticator_data: Base64UrlSafeData::from(vec![
                    239, 115, 241, 111, 91, 226, 27, 23, 185, 145, 15, 75, 208, 190, 109, 73, 186,
                    119, 107, 122, 2, 224, 117, 140, 139, 132, 92, 21, 148, 105, 187, 55, 1, 0, 0,
                    3, 237,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 103, 101, 116, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110, 103, 101, 34,
                    58, 34, 102, 122, 84, 81, 56, 57, 90, 89, 84, 121, 73, 77, 52, 112, 72, 90, 50,
                    102, 70, 106, 53, 75, 118, 111, 52, 104, 113, 95, 73, 72, 111, 69, 112, 78,
                    107, 120, 104, 108, 87, 104, 100, 67, 65, 34, 44, 34, 111, 114, 105, 103, 105,
                    110, 34, 58, 34, 104, 116, 116, 112, 115, 58, 47, 47, 115, 117, 98, 46, 105,
                    100, 109, 46, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109, 58, 56, 48,
                    56, 48, 34, 44, 34, 99, 114, 111, 115, 115, 79, 114, 105, 103, 105, 110, 34,
                    58, 102, 97, 108, 115, 101, 125,
                ]),
                signature: Base64UrlSafeData::from(vec![
                    48, 69, 2, 32, 113, 175, 47, 74, 251, 87, 115, 175, 144, 222, 52, 128, 21, 250,
                    35, 239, 213, 162, 75, 45, 110, 28, 15, 103, 138, 234, 106, 219, 34, 198, 74,
                    74, 2, 33, 0, 204, 144, 147, 62, 250, 6, 11, 19, 239, 90, 108, 6, 126, 165,
                    157, 41, 223, 251, 81, 22, 202, 121, 126, 133, 192, 81, 71, 193, 220, 208, 25,
                    127,
                ]),
                user_handle: Some(Base64UrlSafeData::from(vec![])),
            },
            extensions: AuthenticationExtensionsClientOutputs::default(),
            type_: "public-key".to_string(),
        };

        let r = wan.verify_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &cred,
            &None,
            false,
        );
        trace!("RESULT: {:?}", r);
        assert!(r.is_ok());
    }

    #[test]
    fn test_yk5bio_fallback_alg_attest_none() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "http://localhost:8080/auth",
            "localhost",
            vec![Url::parse("http://localhost:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"NE6dm0mgUe47-X0Yf5nRdhYokY3A8XAzs10KBLGlVY0\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(r#"{
            "id": "k8-N3sbgQe_ze58s5b955iLRrqcizmms-YOqFQTQbBbbJLStt9CaR3vUYXEajy4O22fAgdyY1aOvc6HW9o1ikqiSWee2CxXXJe2DE40byI4-m4oesHfmz4urfMxkIrAd_4i8pgWHNLVlTSMtAzhCXH16Yw4uUsdsntv1HpYiu94",
            "rawId": "k8-N3sbgQe_ze58s5b955iLRrqcizmms-YOqFQTQbBbbJLStt9CaR3vUYXEajy4O22fAgdyY1aOvc6HW9o1ikqiSWee2CxXXJe2DE40byI4-m4oesHfmz4urfMxkIrAd_4i8pgWHNLVlTSMtAzhCXH16Yw4uUsdsntv1HpYiu94",
            "response": {
                "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVjhSZYN5YgOjGh0NBcPZHZgW4_krrmihjLHmVzzuoMdl2NFAAAAAQAAAAAAAAAAAAAAAAAAAAAAgJPPjd7G4EHv83ufLOW_eeYi0a6nIs5prPmDqhUE0GwW2yS0rbfQmkd71GFxGo8uDttnwIHcmNWjr3Oh1vaNYpKoklnntgsV1yXtgxONG8iOPpuKHrB35s-Lq3zMZCKwHf-IvKYFhzS1ZU0jLQM4Qlx9emMOLlLHbJ7b9R6WIrvepAEBAycgBiFYICgd3qEI_iQqhYAi0y47WqeU2Bf2kVY4Mq02t1zgTzkV",
                "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiTkU2ZG0wbWdVZTQ3LVgwWWY1blJkaFlva1kzQThYQXpzMTBLQkxHbFZZMCIsIm9yaWdpbiI6Imh0dHA6Ly9sb2NhbGhvc3Q6ODA4MCIsImNyb3NzT3JpZ2luIjpmYWxzZX0"
            },
            "type": "public-key"
        }"#).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[COSEAlgorithm::EDDSA],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        debug!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_solokey_fallback_alg_attest_none() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"rRPXQ7lps3xBQzX3dDAor9fHwH_ff55gUU-8wwZVK-g\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(r#"{
            "id": "owBY6NCpGj_5nAM427VzsWjmifVdW10z3Ov8fyN5BPX5cxyR2umlVN5h7oGUos-9RPeoYBuCRBkSyAK6jM0gkZ0RLrHrCGRTwfk5p1NQ2ucX_cAh0uel-TkBpyWE-dxqXyk-WLlhSA4LKEdlmyTVqiDAGG7CRHdDn0oAufgq0za7-Crt6cWPKwzmkTGHsMAaEqEaQzHjo1D-pb_WkJJfYp5SZ52ZdTj5eKx7htT5QIogb70lwTKv82ix8PZskqiV-L4j5EroU-xXl7sxKlVtmkS8tSlHpyU-h8fZcFmmW4lr6cBOACd5aNEgR88BTFqQQZ97RORZ7J9sagJQJ63Jj-CZTqGBewVu2jazgA",
            "rawId": "owBY6NCpGj_5nAM427VzsWjmifVdW10z3Ov8fyN5BPX5cxyR2umlVN5h7oGUos-9RPeoYBuCRBkSyAK6jM0gkZ0RLrHrCGRTwfk5p1NQ2ucX_cAh0uel-TkBpyWE-dxqXyk-WLlhSA4LKEdlmyTVqiDAGG7CRHdDn0oAufgq0za7-Crt6cWPKwzmkTGHsMAaEqEaQzHjo1D-pb_WkJJfYp5SZ52ZdTj5eKx7htT5QIogb70lwTKv82ix8PZskqiV-L4j5EroU-xXl7sxKlVtmkS8tSlHpyU-h8fZcFmmW4lr6cBOACd5aNEgR88BTFqQQZ97RORZ7J9sagJQJ63Jj-CZTqGBewVu2jazgA",
            "response": {
                "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVkBbWq5u_Dfmhb5Hbszu7Ey-vnRfHgsSCbG7HDs7ljZfvUqRQAAAEoAAAAAAAAAAAAAAAAAAAAAAQyjAFjo0KkaP_mcAzjbtXOxaOaJ9V1bXTPc6_x_I3kE9flzHJHa6aVU3mHugZSiz71E96hgG4JEGRLIArqMzSCRnREusesIZFPB-TmnU1Da5xf9wCHS56X5OQGnJYT53GpfKT5YuWFIDgsoR2WbJNWqIMAYbsJEd0OfSgC5-CrTNrv4Ku3pxY8rDOaRMYewwBoSoRpDMeOjUP6lv9aQkl9inlJnnZl1OPl4rHuG1PlAiiBvvSXBMq_zaLHw9mySqJX4viPkSuhT7FeXuzEqVW2aRLy1KUenJT6Hx9lwWaZbiWvpwE4AJ3lo0SBHzwFMWpBBn3tE5Fnsn2xqAlAnrcmP4JlOoYF7BW7aNrOApAEBAycgBiFYIKfpbghX95Ey_8DV4Ots95iyCRWa7OElliqsg9tdnRur",
                "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiclJQWFE3bHBzM3hCUXpYM2REQW9yOWZId0hfZmY1NWdVVS04d3daVkstZyIsIm9yaWdpbiI6Imh0dHBzOi8vd2ViYXV0aG4uZmlyc3R5ZWFyLmlkLmF1IiwiY3Jvc3NPcmlnaW4iOmZhbHNlfQ"
            },
            "type": "public-key"
        }"#).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[COSEAlgorithm::EDDSA],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        debug!("{:?}", result);
        assert!(result.is_ok());
    }

    // ⚠️  Currently IGNORED as it appears that pixel 3a send INVALID attestation requests.
    #[test]
    #[ignore]
    fn test_google_pixel_3a_direct_attestation() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );
        let chal: HumanBinaryData =
            serde_json::from_str("\"Y0j5PX0VXeKb2150k6sAh1QNRBJ3iTv8WBsUfgn_pRs\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(r#"{
            "id": "Afx3PxBAXAercQxfFjvHGt3OnTHtXjfNcuCxI-XVaeAtLkohnHQ_mJ2Ocgj2Bhhkv3neczncwaH1nkVpwitUxyQ",
            "rawId": "Afx3PxBAXAercQxfFjvHGt3OnTHtXjfNcuCxI-XVaeAtLkohnHQ_mJ2Ocgj2Bhhkv3neczncwaH1nkVpwitUxyQ",
            "response": {
                "attestationObject": "o2NmbXRxYW5kcm9pZC1zYWZldHluZXRnYXR0U3RtdKJjdmVyaTIxNDgxNTA0NGhyZXNwb25zZVkgcmV5SmhiR2NpT2lKU1V6STFOaUlzSW5nMVl5STZXeUpOU1VsR1dVUkRRMEpGYVdkQmQwbENRV2RKVWtGT2FHTkhiRGN3UWpWaFNVTlJRVUZCUVVWQ2JpOUZkMFJSV1VwTGIxcEphSFpqVGtGUlJVeENVVUYzVW1wRlRFMUJhMGRCTVZWRlFtaE5RMVpXVFhoSmFrRm5RbWRPVmtKQmIxUkhWV1IyWWpKa2MxcFRRbFZqYmxaNlpFTkNWRnBZU2pKaFYwNXNZM2xDVFZSRlRYaEZla0ZTUW1kT1ZrSkJUVlJEYTJSVlZYbENSRkZUUVhoU1JGRjNTR2hqVGsxcVNYZE5WRWt4VFZSQmQwMUVUVEJYYUdOT1RXcEpkMDVFU1RGTlZFRjNUVVJOZWxkcVFXUk5Vbk4zUjFGWlJGWlJVVVJGZUVwb1pFaFNiR016VVhWWlZ6VnJZMjA1Y0ZwRE5XcGlNakIzWjJkRmFVMUJNRWREVTNGSFUwbGlNMFJSUlVKQlVWVkJRVFJKUWtSM1FYZG5aMFZMUVc5SlFrRlJRMWsxYkhwR1kwaHNaVEZFVEd4MFRrcG9iRk5qYm5GV1VuTllRMWQ2TmpGR2J5OUdSMHRzWW0wMGJHSTVZemR5V1hwWlRtOU1UV3hVV0d0YWFVczBSMUpGZG5acVozZE1kMk0zVEVNNFRUWjZiM0pHY1dFNWFqTjZORzB2VFhWa1EyRkdWblIzTUVGVmJtVnFhbFpTYUZSaVdrVkthV3M0VVVWaWFIZzFZWHBDVGxOd00yZ3JSemcyTlV4YUszbG5SR1JrTUZaYVMyUnhOVE5MUWpscU1FWTRlV0pyWkhaVlkxTnpMMjB6UjAxcVYwVkJhWEEwVjI1eVJGazVSa3hhWm5ncmNFTndRVTVQUVdKVVRuWmphV2xMUVhkUGExRkhSRVZKTVVaeFZFTjFTVzVhYVVoU2RtMXBaazlSYzA5dVUwVjRTWFV6YzFjM2RsRmpSWFJVWWtZclZWcDRhR3BpU0RWRmRtSmtiMFZ1WVV4Tk5sUkNTbmwxYkRkMGVsZDFhalJaTkZoVVkydDJaRk5EYm5KQlUzZHpaM2xST1hWT09YZG9VSFpCVm01NFIxWkNXRWxGVkVWMFZVRTRiWGxRTkROVVMzTktRV2ROUWtGQlIycG5aMHAzVFVsSlEySkVRVTlDWjA1V1NGRTRRa0ZtT0VWQ1FVMURRbUZCZDBWM1dVUldVakJzUWtGM2QwTm5XVWxMZDFsQ1FsRlZTRUYzUlhkRVFWbEVWbEl3VkVGUlNDOUNRVWwzUVVSQlpFSm5UbFpJVVRSRlJtZFJWWEZXVFRKVlRWcFdRVXMxUTNsUldUWkdSM0owVTBrM01YTXliM2RJZDFsRVZsSXdha0pDWjNkR2IwRlZTbVZKV1VSeVNsaHJXbEZ4TldSU1pHaHdRMFF6YkU5NmRVcEpkMkpSV1VsTGQxbENRbEZWU0VGUlJVVlpWRUptVFVOdlIwTkRjMGRCVVZWR1FucEJRbWhvTlc5a1NGSjNUMms0ZG1JeVRucGpRelYzWVRKcmRWb3lPWFphZVRsdVpFaE5lRnBFVW5CaWJsRjNUVkZaU1V0M1dVSkNVVlZJVFVGTFIwcFhhREJrU0VFMlRIazVkMkV5YTNWYU1qbDJXbms1ZVZwWVFuWk1NazVzWTI1U2Vrd3laREJqZWtaclRrTTFhMXBZU1hkSVVWbEVWbEl3VWtKQ1dYZEdTVWxUV1ZoU01GcFlUakJNYlVaMVdraEtkbUZYVVhWWk1qbDBUVU5GUjBFeFZXUkpRVkZoVFVKbmQwTkJXVWRhTkVWTlFWRkpRazFCZDBkRGFYTkhRVkZSUWpGdWEwTkNVVTEzVUhkWlJGWlNNR1pDUkdkM1RtcEJNRzlFUzJkTlNWbDFZVWhTTUdORWIzWk1NazU1WWtoTmRXTkhkSEJNYldSMllqSmpkbG96VW5wTlYxRXdZVmMxTUV3eFNUTlBSMWt4WldwT2NVNHpiRzVNYlU1NVlrUkRRMEZSVFVkRGFYTkhRVkZSUWpGdWEwTkNRVWxGWjJaUlJXZG1SVUUzZDBJeFFVWkhhbk5RV0RsQldHMWpWbTB5TkU0emFWQkVTMUkyZWtKemJua3ZaV1ZwUlV0aFJHWTNWV2wzV0d4QlFVRkNabkJFYkVSQlNVRkJRVkZFUVVWWmQxSkJTV2RKTkRWc1VIRXdOVmRXZUVsNmJ6RlZiR2hvVTBWMmNrbHZRVlkxUlhGME1DdHNWa1Z1YVd4WWNUaFZRMGxEVjNCSFJrZzVSQzlFZVdabllXZFhNeTh5WjBWMVNGcGFPRXRIU3psQ09VcGFla0pEU2l0Q2RsTmxRVWhaUVV0WWJTczRTalExVDFOSWQxWnVUMlpaTmxZek5XSTFXR1phZUdkRGRtbzFWRll3YlZoRFZtUjRORkZCUVVGR0sydFBWVXcwWjBGQlFrRk5RVko2UWtaQmFVVkJiMk50Vm1SamJFTkVNbUpHVUU5T2IxWXlNWFJpT0VkelpWZGtNa1p0TTFkVFIzRlhUVEIzUkRCQ2MwTkpSV1YwUkhsd05YcGpialU0YWpob1VrUlNieTlXVlVkMFp6TnRkaklyV1RaS1JqUnFibnBDVWt0RlVVMUJNRWREVTNGSFUwbGlNMFJSUlVKRGQxVkJRVFJKUWtGUlFVbHViSGh1U1VsMlEwdHJWbWxLWlRWaWRFVTJUVkJaUVdwNE0wZElXakZMTDNwc2RIQnpaVTFTVVRoaVJsVkxUVVpNVTFOeE4zVk9SbEJSY2pkUFZ6Tm9RMmhuVEVORFZtOUZla2MwWW5GR2RVMTRWMklyU0hRNVVFaDBSbmhXV0hwaVowcDVhbUoyUkRkSVUwOVVjV3M0UVZreFlTOU9VVFYxYW5ORFRGTktORVJtTmxKa2FFZ3ZUM1p3ZEdWUU0wNW1iRlZYVGsxSlFrVjJNRlYyTVhSMlRFVm1VVWRYTUdoVFltYzJUQzlJUjJkQlkxZDFURGRzTmk5UVdFbEZkVEpsVERkcllVZEdVbWhKTW1KcU5FcE9PVmxGU0VkdWRtaGpSM0ExTlhsQ016ZG9TWGd4YkRoVk56VllPV2hJTVU4MlRVMXRlblpLTURWeGRGaERjMVJZVVdsbGFrUXdWSFI0VkdwSFZpdFdTM1J3VEZoSlEzQlVabmhPYzNCQ2VrTk1hRGt4U1V4dE1uQkhORlk1Wkd0dFJWWnZPVEIwU25wS1NTOUJTelpoVUdadloyTktiMEpuYm5CVE9GVlpkMEZPYlZORElpd2lUVWxKUm1wRVEwTkJNMU5uUVhkSlFrRm5TVTVCWjBOUGMyZEplazV0VjB4YVRUTmliWHBCVGtKbmEzRm9hMmxIT1hjd1FrRlJjMFpCUkVKSVRWRnpkME5SV1VSV1VWRkhSWGRLVmxWNlJXbE5RMEZIUVRGVlJVTm9UVnBTTWpsMldqSjRiRWxHVW5sa1dFNHdTVVpPYkdOdVduQlpNbFo2U1VWNFRWRjZSVlZOUWtsSFFURlZSVUY0VFV4U01WSlVTVVpLZG1JelVXZFZha1YzU0doalRrMXFRWGRQUkVWNlRVUkJkMDFFVVhsWGFHTk9UV3BqZDA5VVRYZE5SRUYzVFVSUmVWZHFRa2ROVVhOM1ExRlpSRlpSVVVkRmQwcFdWWHBGYVUxRFFVZEJNVlZGUTJoTldsSXlPWFphTW5oc1NVWlNlV1JZVGpCSlJrNXNZMjVhY0ZreVZucEpSWGhOVVhwRlZFMUNSVWRCTVZWRlFYaE5TMUl4VWxSSlJVNUNTVVJHUlU1RVEwTkJVMGwzUkZGWlNrdHZXa2xvZG1OT1FWRkZRa0pSUVVSblowVlFRVVJEUTBGUmIwTm5aMFZDUVV0MlFYRnhVRU5GTWpkc01IYzVla000WkZSUVNVVTRPV0pCSzNoVWJVUmhSemQ1TjFabVVUUmpLMjFQVjJoc1ZXVmlWVkZ3U3pCNWRqSnlOamM0VWtwRmVFc3dTRmRFYW1WeEsyNU1TVWhPTVVWdE5XbzJja0ZTV21sNGJYbFNVMnBvU1ZJd1MwOVJVRWRDVFZWc1pITmhlblJKU1VvM1R6Qm5Memd5Y1dvdmRrZEViQzh2TTNRMGRGUnhlR2xTYUV4UmJsUk1XRXBrWlVJck1rUm9hMlJWTmtsSlozZzJkMDQzUlRWT1kxVklNMUpqYzJWcVkzRnFPSEExVTJveE9YWkNiVFpwTVVab2NVeEhlVzFvVFVaeWIxZFdWVWRQTTNoMFNVZzVNV1J6WjNrMFpVWkxZMlpMVmt4WFN6TnZNakU1TUZFd1RHMHZVMmxMYlV4aVVrbzFRWFUwZVRGbGRVWktiVEpLVFRsbFFqZzBSbXR4WVROcGRuSllWMVZsVm5SNVpUQkRVV1JMZG5OWk1rWnJZWHAyZUhSNGRuVnpURXA2VEZkWlNHczFOWHBqVWtGaFkwUkJNbE5sUlhSQ1lsRm1SREZ4YzBOQmQwVkJRV0ZQUTBGWVdYZG5aMFo1VFVFMFIwRXhWV1JFZDBWQ0wzZFJSVUYzU1VKb2FrRmtRbWRPVmtoVFZVVkdha0ZWUW1kbmNrSm5SVVpDVVdORVFWRlpTVXQzV1VKQ1VWVklRWGRKZDBWbldVUldVakJVUVZGSUwwSkJaM2RDWjBWQ0wzZEpRa0ZFUVdSQ1owNVdTRkUwUlVablVWVktaVWxaUkhKS1dHdGFVWEUxWkZKa2FIQkRSRE5zVDNwMVNrbDNTSGRaUkZaU01HcENRbWQzUm05QlZUVkxPSEpLYmtWaFN6Qm5ibWhUT1ZOYWFYcDJPRWxyVkdOVU5IZGhRVmxKUzNkWlFrSlJWVWhCVVVWRldFUkNZVTFEV1VkRFEzTkhRVkZWUmtKNlFVSm9hSEJ2WkVoU2QwOXBPSFppTWs1NlkwTTFkMkV5YTNWYU1qbDJXbms1Ym1SSVRubE5WRUYzUW1kbmNrSm5SVVpDVVdOM1FXOVphMkZJVWpCalJHOTJURE5DY21GVE5XNWlNamx1VEROS2JHTkhPSFpaTWxaNVpFaE5kbG96VW5wamFrVjFXa2RXZVUxRVVVZEJNVlZrU0hkUmRFMURjM2RMWVVGdWIwTlhSMGt5YURCa1NFRTJUSGs1YW1OdGQzVmpSM1J3VEcxa2RtSXlZM1phTTFKNlkycEZkbG96VW5wamFrVjFXVE5LYzAxRk1FZEJNVlZrU1VGU1IwMUZVWGREUVZsSFdqUkZUVUZSU1VKTlJHZEhRMmx6UjBGUlVVSXhibXREUWxGTmQwdHFRVzlDWjJkeVFtZEZSa0pSWTBOQlVsbGpZVWhTTUdOSVRUWk1lVGwzWVRKcmRWb3lPWFphZVRsNVdsaENkbU15YkRCaU0wbzFUSHBCVGtKbmEzRm9hMmxIT1hjd1FrRlJjMFpCUVU5RFFXZEZRVWxXVkc5NU1qUnFkMWhWY2pCeVFWQmpPVEkwZG5WVFZtSkxVWFZaZHpOdVRHWnNUR1pNYURWQldWZEZaVlpzTDBSMU1UaFJRVmRWVFdSalNqWnZMM0ZHV21Kb1dHdENTREJRVG1OM09UZDBhR0ZtTWtKbGIwUlpXVGxEYXk5aUsxVkhiSFZvZURBMmVtUTBSVUptTjBnNVVEZzBibTV5ZDNCU0t6UkhRa1JhU3l0WWFETkpNSFJ4U25reWNtZFBjVTVFWm14eU5VbE5VVGhhVkZkQk0zbHNkR0ZyZWxOQ1MxbzJXSEJHTUZCd2NYbERVblp3TDA1RFIzWXlTMWd5VkhWUVEwcDJjMk53TVM5dE1uQldWSFI1UW1wWlVGSlJLMUYxUTFGSFFVcExhblJPTjFJMVJFWnlabFJ4VFZkMldXZFdiSEJEU2tKcmQyeDFOeXMzUzFrelkxUkpabnBGTjJOdFFVeHphMDFMVGt4MVJIb3JVbnBEWTNOWlZITldZVlUzVm5BemVFdzJNRTlaYUhGR2EzVkJUMDk0UkZvMmNFaFBhamtyVDBwdFdXZFFiVTlVTkZnekt6ZE1OVEZtV0VwNVVrZzVTMlpNVWxBMmJsUXpNVVExYm0xelIwRlBaMW95Tmk4NFZEbG9jMEpYTVhWdk9XcDFOV1phVEZwWVZsWlROVWd3U0hsSlFrMUZTM2xIVFVsUWFFWlhjbXgwTDJoR1V6STRUakY2WVV0Sk1GcENSMFF6WjFsblJFeGlhVVJVT1daSFdITjBjR3NyUm0xak5HOXNWbXhYVUhwWVpUZ3hkbVJ2Ulc1R1luSTFUVEkzTWtoa1owcFhieXRYYUZRNVFsbE5NRXBwSzNka1ZtMXVVbVptV0dkc2IwVnZiSFZVVG1OWGVtTTBNV1JHY0dkS2RUaG1Sak5NUnpCbmJESnBZbE5aYVVOcE9XRTJhSFpWTUZSd2NHcEtlVWxYV0doclNsUmpUVXBzVUhKWGVERldlWFJGVlVkeVdESnNNRXBFZDFKcVZ5ODJOVFp5TUV0V1FqQXllRWhTUzNadE1scExTVEF6Vkdkc1RFbHdiVlpEU3pOclFrdHJTMDV3UWs1clJuUTRjbWhoWm1ORFMwOWlPVXA0THpsMGNFNUdiRkZVYkRkQ016bHlTbXhLVjJ0U01UZFJibHB4Vm5CMFJtVlFSazlTYjFwdFJucE5QU0lzSWsxSlNVWlpha05EUWtWeFowRjNTVUpCWjBsUlpEY3dUbUpPY3pJclVuSnhTVkV2UlRoR2FsUkVWRUZPUW1kcmNXaHJhVWM1ZHpCQ1FWRnpSa0ZFUWxoTlVYTjNRMUZaUkZaUlVVZEZkMHBEVWxSRldrMUNZMGRCTVZWRlEyaE5VVkl5ZUhaWmJVWnpWVEpzYm1KcFFuVmthVEY2V1ZSRlVVMUJORWRCTVZWRlEzaE5TRlZ0T1haa1EwSkVVVlJGWWsxQ2EwZEJNVlZGUVhoTlUxSXllSFpaYlVaelZUSnNibUpwUWxOaU1qa3dTVVZPUWsxQ05GaEVWRWwzVFVSWmVFOVVRWGROUkVFd1RXeHZXRVJVU1RSTlJFVjVUMFJCZDAxRVFUQk5iRzkzVW5wRlRFMUJhMGRCTVZWRlFtaE5RMVpXVFhoSmFrRm5RbWRPVmtKQmIxUkhWV1IyWWpKa2MxcFRRbFZqYmxaNlpFTkNWRnBZU2pKaFYwNXNZM2xDVFZSRlRYaEdSRUZUUW1kT1ZrSkJUVlJETUdSVlZYbENVMkl5T1RCSlJrbDRUVWxKUTBscVFVNUNaMnR4YUd0cFJ6bDNNRUpCVVVWR1FVRlBRMEZuT0VGTlNVbERRMmRMUTBGblJVRjBhRVZEYVhnM2FtOVlaV0pQT1hrdmJFUTJNMnhoWkVGUVMwZzVaM1pzT1UxbllVTmpabUl5YWtndk56Wk9kVGhoYVRaWWJEWlBUVk12YTNJNWNrZzFlbTlSWkhObWJrWnNPVGQyZFdaTGFqWmlkMU5wVmpadWNXeExjaXREVFc1NU5sTjRia2RRWWpFMWJDczRRWEJsTmpKcGJUbE5XbUZTZHpGT1JVUlFhbFJ5UlZSdk9HZFpZa1YyY3k5QmJWRXpOVEZyUzFOVmFrSTJSekF3YWpCMVdVOUVVREJuYlVoMU9ERkpPRVV6UTNkdWNVbHBjblUyZWpGcldqRnhLMUJ6UVdWM2JtcEllR2R6U0VFemVUWnRZbGQzV2tSeVdGbG1hVmxoVWxGTk9YTkliV3RzUTJsMFJETTRiVFZoWjBrdmNHSnZVRWRwVlZVck5rUlBiMmR5UmxwWlNuTjFRalpxUXpVeE1YQjZjbkF4V210cU5WcFFZVXMwT1d3NFMwVnFPRU00VVUxQlRGaE1NekpvTjAweFlrdDNXVlZJSzBVMFJYcE9hM1JOWnpaVVR6aFZjRzEyVFhKVmNITjVWWEYwUldvMVkzVklTMXBRWm0xbmFFTk9Oa296UTJsdmFqWlBSMkZMTDBkUU5VRm1iRFF2V0hSalpDOXdNbWd2Y25Nek4wVlBaVnBXV0hSTU1HMDNPVmxDTUdWelYwTnlkVTlETjFoR2VGbHdWbkU1VDNNMmNFWk1TMk4zV25CRVNXeFVhWEo0V2xWVVVVRnpObkY2YTIwd05uQTVPR2MzUWtGbEsyUkVjVFprYzI4ME9UbHBXVWcyVkV0WUx6RlpOMFI2YTNabmRHUnBlbXByV0ZCa2MwUjBVVU4yT1ZWM0szZHdPVlUzUkdKSFMyOW5VR1ZOWVROTlpDdHdkbVY2TjFjek5VVnBSWFZoS3l0MFoza3ZRa0pxUmtaR2VUTnNNMWRHY0U4NVMxZG5lamQ2Y0cwM1FXVkxTblE0VkRFeFpHeGxRMlpsV0d0clZVRkxTVUZtTlhGdlNXSmhjSE5hVjNkd1ltdE9SbWhJWVhneWVFbFFSVVJuWm1jeFlYcFdXVGd3V21OR2RXTjBURGRVYkV4dVRWRXZNR3hWVkdKcFUzY3hia2cyT1UxSE5ucFBNR0k1WmpaQ1VXUm5RVzFFTURaNVN6VTJiVVJqV1VKYVZVTkJkMFZCUVdGUFEwRlVaM2RuWjBVd1RVRTBSMEV4VldSRWQwVkNMM2RSUlVGM1NVSm9ha0ZRUW1kT1ZraFNUVUpCWmpoRlFsUkJSRUZSU0M5TlFqQkhRVEZWWkVSblVWZENRbFJyY25semJXTlNiM0pUUTJWR1RERktiVXhQTDNkcFVrNTRVR3BCWmtKblRsWklVMDFGUjBSQlYyZENVbWRsTWxsaFVsRXlXSGx2YkZGTU16QkZlbFJUYnk4dmVqbFRla0puUW1kbmNrSm5SVVpDVVdOQ1FWRlNWVTFHU1hkS1VWbEpTM2RaUWtKUlZVaE5RVWRIUjFkb01HUklRVFpNZVRsMldUTk9kMHh1UW5KaFV6VnVZakk1Ymt3eVpIcGpha1YzUzFGWlNVdDNXVUpDVVZWSVRVRkxSMGhYYURCa1NFRTJUSGs1ZDJFeWEzVmFNamwyV25rNWJtTXpTWGhNTW1SNlkycEZkVmt6U2pCTlJFbEhRVEZWWkVoM1VYSk5RMnQzU2paQmJHOURUMGRKVjJnd1pFaEJOa3g1T1dwamJYZDFZMGQwY0V4dFpIWmlNbU4yV2pOT2VVMVRPVzVqTTBsNFRHMU9lV0pFUVRkQ1owNVdTRk5CUlU1RVFYbE5RV2RIUW0xbFFrUkJSVU5CVkVGSlFtZGFibWRSZDBKQlowbDNSRkZaVEV0M1dVSkNRVWhYWlZGSlJrRjNTWGRFVVZsTVMzZFpRa0pCU0ZkbFVVbEdRWGROZDBSUldVcExiMXBKYUhaalRrRlJSVXhDVVVGRVoyZEZRa0ZFVTJ0SWNrVnZiemxETUdSb1pXMU5XRzlvTm1SR1UxQnphbUprUWxwQ2FVeG5PVTVTTTNRMVVDdFVORlo0Wm5FM2RuRm1UUzlpTlVFelVta3habmxLYlRsaWRtaGtSMkZLVVROaU1uUTJlVTFCV1U0dmIyeFZZWHB6WVV3cmVYbEZiamxYY0hKTFFWTlBjMmhKUVhKQmIzbGFiQ3QwU21GdmVERXhPR1psYzNOdFdHNHhhRWxXZHpReGIyVlJZVEYyTVhabk5FWjJOelI2VUd3MkwwRm9VM0ozT1ZVMWNFTmFSWFEwVjJrMGQxTjBlalprVkZvdlEweEJUbmc0VEZwb01VbzNVVXBXYWpKbWFFMTBabFJLY2psM05Ib3pNRm95TURsbVQxVXdhVTlOZVN0eFpIVkNiWEIyZGxsMVVqZG9Xa3cyUkhWd2MzcG1ibmN3VTJ0bWRHaHpNVGhrUnpsYVMySTFPVlZvZG0xaFUwZGFVbFppVGxGd2MyY3pRbHBzZG1sa01HeEpTMDh5WkRGNGIzcGpiRTk2WjJwWVVGbHZka3BLU1hWc2RIcHJUWFV6TkhGUllqbFRlaTk1YVd4eVlrTm5hamc5SWwxOS5leUp1YjI1alpTSTZJbko1VFZZMVRpdEpTVzlPYzBnNE9YTk1NbXhCWkRKRWIxaEVUMFZaVFRsQlZGQjJSblJuZW1KVGIwMDlJaXdpZEdsdFpYTjBZVzF3VFhNaU9qRTJORFEzTXpnek9EVTNPVFFzSW1Gd2ExQmhZMnRoWjJWT1lXMWxJam9pWTI5dExtZHZiMmRzWlM1aGJtUnliMmxrTG1kdGN5SXNJbUZ3YTBScFoyVnpkRk5vWVRJMU5pSTZJbVpaUlVSV2VVbHFPRFJ4V2xkd1dXazBRMUJ6VlU4MlN6aHVZbU5RWWs0dlkwczJXREl3UTJSM09GVTlJaXdpWTNSelVISnZabWxzWlUxaGRHTm9JanAwY25WbExDSmhjR3REWlhKMGFXWnBZMkYwWlVScFoyVnpkRk5vWVRJMU5pSTZXeUk0VURGelZ6QkZVRXBqYzJ4M04xVjZVbk5wV0V3Mk5IY3JUelV3UldRclVrSkpRM1JoZVRGbk1qUk5QU0pkTENKaVlYTnBZMGx1ZEdWbmNtbDBlU0k2ZEhKMVpTd2laWFpoYkhWaGRHbHZibFI1Y0dVaU9pSkNRVk5KUXl4SVFWSkVWMEZTUlY5Q1FVTkxSVVFpZlEuYXZwSHpzT2VCUlEydUVLLXdNc2oyam5BX19iY19nd2dWYTladlAxMGhrbC1fYVZYb2I5aF9PN2JwTlpZRWR6VjI1VVR4X1BQRzFPMHpiNG9oLUo0TDZwam0yMGZZclRXTndZeGJaLWxYamRZcW1YWmsybkxLMnJTWkZNOWxyVTJGOXJvOUdSOEtsN3JzenpxazBQa3N1NkFybzgtRTRlWGoxQ3ZGYnB6cEQ1VUVZeXp0M0JaUE9KWTZYVVU1LXd2azV1UFl2OWhCeG5jNEdPYXdRelJiY3l3Ukh6N2g1NWMwV2dqVUNpOFc2SD04xVjZVbk5wV0V3Mk5IY3JUelV3UldRclVrSkpRM1JoZVRGbk1qUk5QU0pkTENKaVlYTnBZMGx1ZEdWbmNtbDBlU0k2ZEhKMVpTd2laWFpoYkhWaGRHbHZibFI1Y0dVaU9pSkNRVk5KUXl4SVFWSkVWMEZTUlY5Q1FVTkxSVVFpZlEuYXZwSHpzT2VCUlEydUVLLXdNc2oyam5BX19iY19nd2dWYTladlAxMGhrbC1fYVZYb2I5aF9PN2JwTlpZRWR6VjI1VVR4X1BQRzFPMHpiNG9oLUo0TDZwam0yMGZZclRXTndZeGJaLWxYamRZcW1YWmsybkxLMnJTWkZNOWxyVTJGOXJvOUdSOEtsN3JzenpxazBQa3N1NkFybzgtRTRlWGoxQ3ZGYnB6cEQ1VUVZeXp0M0JaUE9KWTZYVVU1LXd2azV1UFl2OWhCeG5jNEdPYXdRelJiY3l3Ukh6N2g1NWMwV2dqVUNpOFc2SD04xVjZVbk5wV0V3Mk5IY3JUelV3UldRclVrSkpRM1JoZjhXQnNVZmduX3BScyIsIm9yaWdpbiI6Imh0dHBzOlwvXC93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJhbmRyb2lkUGFja2FnZU5hbWUiOiJjb20uYW5kcm9pZC5jaHJvbWUifQ"
            },
            "type": "public-key"
        }"#).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        debug!("{:?}", result);
        // Currently UNSUPPORTED as openssl doesn't have eddsa management utils that we need.
        assert!(result.is_err());
    }

    #[test]
    fn test_google_pixel_3a_none_attestation() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"55Wztjbgks9UkS5jYthawNFik0HSiYuCSB5pzNbT6k0\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(r#"{
        "id": "AfzEi3UOVveYjwUwIFO3QuN9V0fomECvAYrD_8S5FAsUJqtGbwpgB9bEfphVOURzFQoEszkuULIj5fMvnTkt6cs",
        "rawId": "AfzEi3UOVveYjwUwIFO3QuN9V0fomECvAYrD_8S5FAsUJqtGbwpgB9bEfphVOURzFQoEszkuULIj5fMvnTkt6cs",
        "response": {
          "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVjFarm78N-aFvkduzO7sTL6-dF8eCxIJsbscOzuWNl-9SpFAAAAAAAAAAAAAAAAAAAAAAAAAAAAQQH8xIt1Dlb3mI8FMCBTt0LjfVdH6JhArwGKw__EuRQLFCarRm8KYAfWxH6YVTlEcxUKBLM5LlCyI-XzL505LenLpQECAyYgASFYII2OFisY2sjerzLYjLYvHsQh8V7cnpRcSL4A77wKqcRTIlggm7s0CUKEmkBBFp7Nng-9_pZ5Dm9y39uy6QJmDLgmgho",
          "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiNTVXenRqYmdrczlVa1M1all0aGF3TkZpazBIU2lZdUNTQjVwek5iVDZrMCIsIm9yaWdpbiI6Imh0dHBzOlwvXC93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJhbmRyb2lkUGFja2FnZU5hbWUiOiJjb20uYW5kcm9pZC5jaHJvbWUifQ"
        },
        "type": "public-key"
        }"#).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            true,
        );
        debug!("{:?}", result);
        assert!(result.is_ok());
    }

    #[test]
    fn test_google_pixel_3a_ignores_requested_algo() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"t_We131NpwllyPL0x26bzZgkF5f_XvA7Ocb4b98zlxM\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(r#"{
        "id": "AfJfonHsXY_f7_gFmV1dI473Ce--_g0tHhdXUoh7JmMn0gzhYUtU9bFqpCgSljjwJxEXkjzb-11ulePZyI0RiyQ",
        "rawId": "AfJfonHsXY_f7_gFmV1dI473Ce--_g0tHhdXUoh7JmMn0gzhYUtU9bFqpCgSljjwJxEXkjzb-11ulePZyI0RiyQ",
        "response": {
            "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVjFarm78N-aFvkduzO7sTL6-dF8eCxIJsbscOzuWNl-9SpFAAAAAAAAAAAAAAAAAAAAAAAAAAAAQQHyX6Jx7F2P3-_4BZldXSOO9wnvvv4NLR4XV1KIeyZjJ9IM4WFLVPWxaqQoEpY48CcRF5I82_tdbpXj2ciNEYskpQECAyYgASFYIE_9awy66uhXZ6hIzPAW2AzIrTMZ7kyC2jtZe0zuH_pOIlggFbNKhOSt8-prIx0snKRqcxULtc2u1rzUUf47g1PxTcU",
            "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoidF9XZTEzMU5wd2xseVBMMHgyNmJ6WmdrRjVmX1h2QTdPY2I0Yjk4emx4TSIsIm9yaWdpbiI6Imh0dHBzOlwvXC93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJhbmRyb2lkUGFja2FnZU5hbWUiOiJvcmcubW96aWxsYS5maXJlZm94In0"
        },
        "type": "public-key"
        }"#).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[
                COSEAlgorithm::RS256,
                COSEAlgorithm::EDDSA,
                COSEAlgorithm::INSECURE_RS1,
            ],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        debug!("{:?}", result);
        assert!(result.is_err());
    }

    /// See https://github.com/kanidm/webauthn-rs/issues/105
    #[test]
    fn test_firefox_98_hello_incorrectly_truncates_aaguid() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "https://webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"FKVseWmr5DxQ_H9iTyoTgRPIClLspXO0XbOKQfMuaFc\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(r#"{
            "id": "6h7wVk2n4Buulhd5fiShGb0BBViIgvDoVO3xhn0A0Mg",
            "rawId": "6h7wVk2n4Buulhd5fiShGb0BBViIgvDoVO3xhn0A0Mg",
            "response": {
            "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVkBWGq5u_Dfmhb5Hbszu7Ey-vnRfHgsSCbG7HDs7ljZfvUqRQAAAAAAACDqHvBWTafgG66WF3l-JKEZvQEFWIiC8OhU7fGGfQDQyKQBAwM5AQAgWQEAt86lR2w_hmnhDr6tvJD5hmIuWt0QkG1sphC8aqeOHuIWnbcBWnxNUrKQibJxEGJilM20s-_w-aUjDoV5MYu4NBgguFHju-qA-qe1sjhqY7UkMkx4Z1KGMeiZNNGgk5Gtmu0xjaq-1RohB3TKADeWTularHWzG6q6sJHgC-qKKa67Rmwr0T4a4S3VjLvjvSPILx88nLJvwqO1rDb5cLOgL5CEjtRijR6SNeN05uBhz2ePn5mMo2lN73pHsMGPo68pGWIWWsb2sC_aBF2eA02Me2jldIgSzMy3y8xsTIg6r_xF105pC8jOPsQVN2TJDxN9zVEuxpY_mUsqGOAFGR-SiyFDAQAB",
            "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJGS1ZzZVdtcjVEeFFfSDlpVHlvVGdSUElDbExzcFhPMFhiT0tRZk11YUZjIiwiY2xpZW50RXh0ZW5zaW9ucyI6e30sImhhc2hBbGdvcml0aG0iOiJTSEEtMjU2Iiwib3JpZ2luIjoiaHR0cHM6Ly93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIn0"
            },
            "type": "public-key"
        }"#).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[
                COSEAlgorithm::RS256,
                COSEAlgorithm::EDDSA,
                COSEAlgorithm::INSECURE_RS1,
            ],
            None,
            false,
            &RequestRegistrationExtensions::default(),
            false,
        );
        debug!("{:?}", result);
        assert!(matches!(result, Err(WebauthnError::ParseNOMFailure)));
    }

    #[test]
    fn test_edge_touchid_rk_verified() {
        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "http://localhost:8080/auth",
            "localhost",
            vec![Url::parse("http://localhost:8080").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData = HumanBinaryData::from(vec![
            108, 33, 62, 167, 162, 234, 36, 63, 176, 231, 161, 58, 41, 233, 117, 157, 210, 244,
            123, 28, 194, 100, 34, 68, 32, 1, 183, 240, 100, 225, 182, 48,
        ]);
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = RegisterPublicKeyCredential {
            id: "AWtT-NSYHNmZjP2R9JAbBmwf3sWMxs_L4_O2XoIvI8HY-rGPjA".to_string(),
            raw_id: Base64UrlSafeData::from(vec![
                1, 107, 83, 248, 212, 152, 28, 217, 153, 140, 253, 145, 244, 144, 27, 6, 108, 31,
                222, 197, 140, 198, 207, 203, 227, 243, 182, 94, 130, 47, 35, 193, 216, 250, 177,
                143, 140,
            ]),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: Base64UrlSafeData::from(vec![
                    163, 99, 102, 109, 116, 102, 112, 97, 99, 107, 101, 100, 103, 97, 116, 116, 83,
                    116, 109, 116, 162, 99, 97, 108, 103, 38, 99, 115, 105, 103, 88, 72, 48, 70, 2,
                    33, 0, 234, 66, 128, 149, 10, 78, 90, 6, 183, 58, 163, 114, 112, 146, 47, 204,
                    176, 27, 86, 218, 77, 135, 121, 88, 40, 94, 115, 7, 221, 248, 13, 37, 2, 33, 0,
                    187, 63, 74, 17, 114, 129, 51, 239, 145, 128, 216, 117, 39, 191, 130, 6, 239,
                    79, 15, 80, 58, 52, 18, 24, 57, 174, 125, 198, 248, 46, 138, 177, 104, 97, 117,
                    116, 104, 68, 97, 116, 97, 88, 169, 73, 150, 13, 229, 136, 14, 140, 104, 116,
                    52, 23, 15, 100, 118, 96, 91, 143, 228, 174, 185, 162, 134, 50, 199, 153, 92,
                    243, 186, 131, 29, 151, 99, 69, 98, 76, 219, 31, 173, 206, 0, 2, 53, 188, 198,
                    10, 100, 139, 11, 37, 241, 240, 85, 3, 0, 37, 1, 107, 83, 248, 212, 152, 28,
                    217, 153, 140, 253, 145, 244, 144, 27, 6, 108, 31, 222, 197, 140, 198, 207,
                    203, 227, 243, 182, 94, 130, 47, 35, 193, 216, 250, 177, 143, 140, 165, 1, 2,
                    3, 38, 32, 1, 33, 88, 32, 143, 255, 51, 238, 28, 38, 130, 245, 24, 48, 164,
                    117, 49, 102, 142, 103, 25, 46, 253, 137, 228, 16, 220, 131, 17, 229, 52, 165,
                    75, 224, 218, 237, 34, 88, 32, 115, 152, 43, 120, 40, 171, 135, 110, 112, 253,
                    28, 142, 154, 9, 9, 149, 94, 254, 147, 235, 38, 4, 215, 26, 217, 51, 245, 151,
                    148, 192, 141, 169,
                ]),
                client_data_json: Base64UrlSafeData::from(vec![
                    123, 34, 116, 121, 112, 101, 34, 58, 34, 119, 101, 98, 97, 117, 116, 104, 110,
                    46, 99, 114, 101, 97, 116, 101, 34, 44, 34, 99, 104, 97, 108, 108, 101, 110,
                    103, 101, 34, 58, 34, 98, 67, 69, 45, 112, 54, 76, 113, 74, 68, 45, 119, 53,
                    54, 69, 54, 75, 101, 108, 49, 110, 100, 76, 48, 101, 120, 122, 67, 90, 67, 74,
                    69, 73, 65, 71, 51, 56, 71, 84, 104, 116, 106, 65, 34, 44, 34, 111, 114, 105,
                    103, 105, 110, 34, 58, 34, 104, 116, 116, 112, 58, 47, 47, 108, 111, 99, 97,
                    108, 104, 111, 115, 116, 58, 56, 48, 56, 48, 34, 44, 34, 99, 114, 111, 115,
                    115, 79, 114, 105, 103, 105, 110, 34, 58, 102, 97, 108, 115, 101, 125,
                ]),
                transports: None,
            },
            type_: "public-key".to_string(),
            extensions: RegistrationExtensionsClientOutputs::default(),
        };

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            // Some(&AttestationCaList {
            //    cas: vec![AttestationCa::apple_webauthn_root_ca()],
            // }),
            true,
            &RequestRegistrationExtensions::default(),
            true,
        );
        debug!("{:?}", result);
        let cred = result.unwrap();
        assert!(matches!(
            cred.attestation.data,
            ParsedAttestationData::Self_
        ));
    }

    #[test]
    fn test_google_safetynet() {
        #[allow(unused)]
        let _request = r#"{"publicKey": {
            "challenge": "dfo+HlqJp3MLK+J5TLxxmvXJieS3zGwdk9G9H9bPezg=",
            "rp": {
                "name": "webauthn.io",
                "id": "webauthn.io"
            },
            "user": {
                "name": "safetynetter",
                "displayName": "safetynetter",
                "id": "wDkAAAAAAAAAAA=="
            },
            "pubKeyCredParams": [
                {
                "type": "public-key",
                "alg": -7
                }
            ],
            "authenticatorSelection": {
                "authenticatorAttachment": "platform",
                "userVerification": "preferred"
            },
            "timeout": 60000,
            "attestation": "direct"
            }
        }"#;

        let response = r#"{
            "id":"AUiVU3Mk3uJomfHcJcu6ScwUHRysE2e6IgaTNAzQ34TP0OPifi2LgGD_5hzxRhOfQTB1fW6k63C8tk-MwywpNVI",
            "rawId":"AUiVU3Mk3uJomfHcJcu6ScwUHRysE2e6IgaTNAzQ34TP0OPifi2LgGD_5hzxRhOfQTB1fW6k63C8tk-MwywpNVI",
            "type":"public-key",
            "response":{
                "attestationObject":"o2NmbXRxYW5kcm9pZC1zYWZldHluZXRnYXR0U3RtdKJjdmVyaDE1MTgwMDM3aHJlc3BvbnNlWRS9ZXlKaGJHY2lPaUpTVXpJMU5pSXNJbmcxWXlJNld5Sk5TVWxHYTJwRFEwSkljV2RCZDBsQ1FXZEpVVkpZY205T01GcFBaRkpyUWtGQlFVRkJRVkIxYm5wQlRrSm5hM0ZvYTJsSE9YY3dRa0ZSYzBaQlJFSkRUVkZ6ZDBOUldVUldVVkZIUlhkS1ZsVjZSV1ZOUW5kSFFURlZSVU5vVFZaU01qbDJXako0YkVsR1VubGtXRTR3U1VaT2JHTnVXbkJaTWxaNlRWSk5kMFZSV1VSV1VWRkVSWGR3U0ZaR1RXZFJNRVZuVFZVNGVFMUNORmhFVkVVMFRWUkJlRTFFUVROTlZHc3dUbFp2V0VSVVJUVk5WRUYzVDFSQk0wMVVhekJPVm05M1lrUkZURTFCYTBkQk1WVkZRbWhOUTFaV1RYaEZla0ZTUW1kT1ZrSkJaMVJEYTA1b1lrZHNiV0l6U25WaFYwVjRSbXBCVlVKblRsWkNRV05VUkZVeGRtUlhOVEJaVjJ4MVNVWmFjRnBZWTNoRmVrRlNRbWRPVmtKQmIxUkRhMlIyWWpKa2MxcFRRazFVUlUxNFIzcEJXa0puVGxaQ1FVMVVSVzFHTUdSSFZucGtRelZvWW0xU2VXSXliR3RNYlU1MllsUkRRMEZUU1hkRVVWbEtTMjlhU1doMlkwNUJVVVZDUWxGQlJHZG5SVkJCUkVORFFWRnZRMmRuUlVKQlRtcFlhM293WlVzeFUwVTBiU3N2UnpWM1QyOHJXRWRUUlVOeWNXUnVPRGh6UTNCU04yWnpNVFJtU3pCU2FETmFRMWxhVEVaSWNVSnJOa0Z0V2xaM01rczVSa2N3VHpseVVsQmxVVVJKVmxKNVJUTXdVWFZ1VXpsMVowaEROR1ZuT1c5MmRrOXRLMUZrV2pKd09UTllhSHAxYmxGRmFGVlhXRU40UVVSSlJVZEtTek5UTW1GQlpucGxPVGxRVEZNeU9XaE1ZMUYxV1ZoSVJHRkROMDlhY1U1dWIzTnBUMGRwWm5NNGRqRnFhVFpJTDNob2JIUkRXbVV5YkVvck4wZDFkSHBsZUV0d2VIWndSUzkwV2xObVlsazVNRFZ4VTJ4Q2FEbG1jR293TVRWamFtNVJSbXRWYzBGVmQyMUxWa0ZWZFdWVmVqUjBTMk5HU3pSd1pYWk9UR0Y0UlVGc0swOXJhV3hOZEVsWlJHRmpSRFZ1Wld3MGVFcHBlWE0wTVROb1lXZHhWekJYYUdnMVJsQXpPV2hIYXpsRkwwSjNVVlJxWVhwVGVFZGtkbGd3YlRaNFJsbG9hQzh5VmsxNVdtcFVORXQ2VUVwRlEwRjNSVUZCWVU5RFFXeG5kMmRuU2xWTlFUUkhRVEZWWkVSM1JVSXZkMUZGUVhkSlJtOUVRVlJDWjA1V1NGTlZSVVJFUVV0Q1oyZHlRbWRGUmtKUlkwUkJWRUZOUW1kT1ZraFNUVUpCWmpoRlFXcEJRVTFDTUVkQk1WVmtSR2RSVjBKQ1VYRkNVWGRIVjI5S1FtRXhiMVJMY1hWd2J6UlhObmhVTm1veVJFRm1RbWRPVmtoVFRVVkhSRUZYWjBKVFdUQm1hSFZGVDNaUWJTdDRaMjU0YVZGSE5rUnlabEZ1T1V0NlFtdENaMmR5UW1kRlJrSlJZMEpCVVZKWlRVWlpkMHAzV1VsTGQxbENRbEZWU0UxQlIwZEhNbWd3WkVoQk5reDVPWFpaTTA1M1RHNUNjbUZUTlc1aU1qbHVUREprTUdONlJuWk5WRUZ5UW1kbmNrSm5SVVpDVVdOM1FXOVpabUZJVWpCalJHOTJURE5DY21GVE5XNWlNamx1VERKa2VtTnFTWFpTTVZKVVRWVTRlRXh0VG5sa1JFRmtRbWRPVmtoU1JVVkdha0ZWWjJoS2FHUklVbXhqTTFGMVdWYzFhMk50T1hCYVF6VnFZakl3ZDBsUldVUldVakJuUWtKdmQwZEVRVWxDWjFwdVoxRjNRa0ZuU1hkRVFWbExTM2RaUWtKQlNGZGxVVWxHUVhwQmRrSm5UbFpJVWpoRlMwUkJiVTFEVTJkSmNVRm5hR2cxYjJSSVVuZFBhVGgyV1ROS2MweHVRbkpoVXpWdVlqSTVia3d3WkZWVmVrWlFUVk0xYW1OdGQzZG5aMFZGUW1kdmNrSm5SVVZCWkZvMVFXZFJRMEpKU0RGQ1NVaDVRVkJCUVdSM1EydDFVVzFSZEVKb1dVWkpaVGRGTmt4TldqTkJTMUJFVjFsQ1VHdGlNemRxYW1RNE1FOTVRVE5qUlVGQlFVRlhXbVJFTTFCTVFVRkJSVUYzUWtsTlJWbERTVkZEVTFwRFYyVk1Tblp6YVZaWE5rTm5LMmRxTHpsM1dWUktVbnAxTkVocGNXVTBaVmswWXk5dGVYcHFaMGxvUVV4VFlta3ZWR2g2WTNweGRHbHFNMlJyTTNaaVRHTkpWek5NYkRKQ01HODNOVWRSWkdoTmFXZGlRbWRCU0ZWQlZtaFJSMjFwTDFoM2RYcFVPV1ZIT1ZKTVNTdDRNRm95ZFdKNVdrVldla0UzTlZOWlZtUmhTakJPTUVGQlFVWnRXRkU1ZWpWQlFVRkNRVTFCVW1wQ1JVRnBRbU5EZDBFNWFqZE9WRWRZVURJM09IbzBhSEl2ZFVOSWFVRkdUSGx2UTNFeVN6QXJlVXhTZDBwVlltZEpaMlk0WjBocWRuQjNNbTFDTVVWVGFuRXlUMll6UVRCQlJVRjNRMnR1UTJGRlMwWlZlVm8zWmk5UmRFbDNSRkZaU2t0dldrbG9kbU5PUVZGRlRFSlJRVVJuWjBWQ1FVazVibFJtVWt0SlYyZDBiRmRzTTNkQ1REVTFSVlJXTm10aGVuTndhRmN4ZVVGak5VUjFiVFpZVHpReGExcDZkMG8yTVhkS2JXUlNVbFF2VlhORFNYa3hTMFYwTW1Nd1JXcG5iRzVLUTBZeVpXRjNZMFZYYkV4UldUSllVRXg1Um1wclYxRk9ZbE5vUWpGcE5GY3lUbEpIZWxCb2RETnRNV0kwT1doaWMzUjFXRTAyZEZnMVEzbEZTRzVVYURoQ2IyMDBMMWRzUm1sb2VtaG5iamd4Ukd4a2IyZDZMMHN5VlhkTk5sTTJRMEl2VTBWNGEybFdabllyZW1KS01ISnFkbWM1TkVGc1pHcFZabFYzYTBrNVZrNU5ha1ZRTldVNGVXUkNNMjlNYkRabmJIQkRaVVkxWkdkbVUxZzBWVGw0TXpWdmFpOUpTV1F6VlVVdlpGQndZaTl4WjBkMmMydG1aR1Y2ZEcxVmRHVXZTMU50Y21sM1kyZFZWMWRsV0daVVlra3plbk5wYTNkYVltdHdiVkpaUzIxcVVHMW9kalJ5YkdsNlIwTkhkRGhRYmpod2NUaE5Na3RFWmk5UU0ydFdiM1F6WlRFNFVUMGlMQ0pOU1VsRlUycERRMEY2UzJkQmQwbENRV2RKVGtGbFR6QnRjVWRPYVhGdFFrcFhiRkYxUkVGT1FtZHJjV2hyYVVjNWR6QkNRVkZ6UmtGRVFrMU5VMEYzU0dkWlJGWlJVVXhGZUdSSVlrYzVhVmxYZUZSaFYyUjFTVVpLZG1JelVXZFJNRVZuVEZOQ1UwMXFSVlJOUWtWSFFURlZSVU5vVFV0U01uaDJXVzFHYzFVeWJHNWlha1ZVVFVKRlIwRXhWVVZCZUUxTFVqSjRkbGx0Um5OVk1teHVZbXBCWlVaM01IaE9la0V5VFZSVmQwMUVRWGRPUkVwaFJuY3dlVTFVUlhsTlZGVjNUVVJCZDA1RVNtRk5SVWw0UTNwQlNrSm5UbFpDUVZsVVFXeFdWRTFTTkhkSVFWbEVWbEZSUzBWNFZraGlNamx1WWtkVloxWklTakZqTTFGblZUSldlV1J0YkdwYVdFMTRSWHBCVWtKblRsWkNRVTFVUTJ0a1ZWVjVRa1JSVTBGNFZIcEZkMmRuUldsTlFUQkhRMU54UjFOSllqTkVVVVZDUVZGVlFVRTBTVUpFZDBGM1oyZEZTMEZ2U1VKQlVVUlJSMDA1UmpGSmRrNHdOWHByVVU4NUszUk9NWEJKVW5aS2VucDVUMVJJVnpWRWVrVmFhRVF5WlZCRGJuWlZRVEJSYXpJNFJtZEpRMlpMY1VNNVJXdHpRelJVTW1aWFFsbHJMMnBEWmtNelVqTldXazFrVXk5a1RqUmFTME5GVUZwU2NrRjZSSE5wUzFWRWVsSnliVUpDU2pWM2RXUm5lbTVrU1UxWlkweGxMMUpIUjBac05YbFBSRWxMWjJwRmRpOVRTa2d2VlV3clpFVmhiSFJPTVRGQ2JYTkxLMlZSYlUxR0t5dEJZM2hIVG1oeU5UbHhUUzg1YVd3M01Va3laRTQ0UmtkbVkyUmtkM1ZoWldvMFlsaG9jREJNWTFGQ1ltcDRUV05KTjBwUU1HRk5NMVEwU1N0RWMyRjRiVXRHYzJKcWVtRlVUa001ZFhwd1JteG5UMGxuTjNKU01qVjRiM2x1VlhoMk9IWk9iV3R4TjNwa1VFZElXR3Q0VjFrM2IwYzVhaXRLYTFKNVFrRkNhemRZY2twbWIzVmpRbHBGY1VaS1NsTlFhemRZUVRCTVMxY3dXVE42Tlc5Nk1rUXdZekYwU2t0M1NFRm5UVUpCUVVkcVoyZEZlazFKU1VKTWVrRlBRbWRPVmtoUk9FSkJaamhGUWtGTlEwRlpXWGRJVVZsRVZsSXdiRUpDV1hkR1FWbEpTM2RaUWtKUlZVaEJkMFZIUTBOelIwRlJWVVpDZDAxRFRVSkpSMEV4VldSRmQwVkNMM2RSU1UxQldVSkJaamhEUVZGQmQwaFJXVVJXVWpCUFFrSlpSVVpLYWxJclJ6UlJOamdyWWpkSFEyWkhTa0ZpYjA5ME9VTm1NSEpOUWpoSFFURlZaRWwzVVZsTlFtRkJSa3AyYVVJeFpHNUlRamRCWVdkaVpWZGlVMkZNWkM5alIxbFpkVTFFVlVkRFEzTkhRVkZWUmtKM1JVSkNRMnQzU25wQmJFSm5aM0pDWjBWR1FsRmpkMEZaV1ZwaFNGSXdZMFJ2ZGt3eU9XcGpNMEYxWTBkMGNFeHRaSFppTW1OMldqTk9lVTFxUVhsQ1owNVdTRkk0UlV0NlFYQk5RMlZuU21GQmFtaHBSbTlrU0ZKM1QyazRkbGt6U25OTWJrSnlZVk0xYm1JeU9XNU1NbVI2WTJwSmRsb3pUbmxOYVRWcVkyMTNkMUIzV1VSV1VqQm5Ra1JuZDA1cVFUQkNaMXB1WjFGM1FrRm5TWGRMYWtGdlFtZG5ja0puUlVaQ1VXTkRRVkpaWTJGSVVqQmpTRTAyVEhrNWQyRXlhM1ZhTWpsMlduazVlVnBZUW5aak1td3dZak5LTlV4NlFVNUNaMnR4YUd0cFJ6bDNNRUpCVVhOR1FVRlBRMEZSUlVGSGIwRXJUbTV1TnpoNU5uQlNhbVE1V0d4UlYwNWhOMGhVWjJsYUwzSXpVazVIYTIxVmJWbElVRkZ4TmxOamRHazVVRVZoYW5aM1VsUXlhVmRVU0ZGeU1ESm1aWE54VDNGQ1dUSkZWRlYzWjFwUksyeHNkRzlPUm5ab2MwODVkSFpDUTA5SllYcHdjM2RYUXpsaFNqbDRhblUwZEZkRVVVZzRUbFpWTmxsYVdpOVlkR1ZFVTBkVk9WbDZTbkZRYWxrNGNUTk5SSGh5ZW0xeFpYQkNRMlkxYnpodGR5OTNTalJoTWtjMmVIcFZjalpHWWpaVU9FMWpSRTh5TWxCTVVrdzJkVE5OTkZSNmN6TkJNazB4YWpaaWVXdEtXV2s0ZDFkSlVtUkJka3RNVjFwMUwyRjRRbFppZWxsdGNXMTNhMjAxZWt4VFJGYzFia2xCU21KRlRFTlJRMXAzVFVnMU5uUXlSSFp4YjJaNGN6WkNRbU5EUmtsYVZWTndlSFUyZURaMFpEQldOMU4yU2tORGIzTnBjbE50U1dGMGFpODVaRk5UVmtSUmFXSmxkRGh4THpkVlN6UjJORnBWVGpnd1lYUnVXbm94ZVdjOVBTSmRmUS5leUp1YjI1alpTSTZJazlGTDJkV09FYzRXazFKTW1ORUsyRk1lRzB2VGt4a1dVMHdjemxsVDB0V1NYUlhOblZTVDI5d1prRTlJaXdpZEdsdFpYTjBZVzF3VFhNaU9qRTFOVE13TWpnd05ETTFNamtzSW1Gd2ExQmhZMnRoWjJWT1lXMWxJam9pWTI5dExtZHZiMmRzWlM1aGJtUnliMmxrTG1kdGN5SXNJbUZ3YTBScFoyVnpkRk5vWVRJMU5pSTZJbGRVYkd4aVVuVXhZbFEyYlZoeWRXRmlXVWQ1WmtvMFJGUTVVR1I0YnpGUFMwb3ZWRTQzTVZWU1lXODlJaXdpWTNSelVISnZabWxzWlUxaGRHTm9JanAwY25WbExDSmhjR3REWlhKMGFXWnBZMkYwWlVScFoyVnpkRk5vWVRJMU5pSTZXeUk0VURGelZ6QkZVRXBqYzJ4M04xVjZVbk5wV0V3Mk5IY3JUelV3UldRclVrSkpRM1JoZVRGbk1qUk5QU0pkTENKaVlYTnBZMGx1ZEdWbmNtbDBlU0k2ZEhKMVpYMC56V3ViaWlraGt5alhETUJpV080ajZEdnVBZWdpSUh1WGhaNWQtTEh3Z1VBZFVSMWxNTU0tZ0Y4VklmSEdYcFZNZ1hhN3plR0l5NEROU19uNTdBZ2c0eE5lTVhQMHRpMVJ4QktVVlJKeUc1OXVoejJJbDBtZkl1UVZNckRpSHBiWjdYb2tKcG1jZlUyWU9QbmppcjlWUjlsVlRZUHVHV1phT01ua1kyRnlvbTRGZzhrNFA3dEtWWllzTXNERWR3ZVdOdTM5MS1mcXdKWUxQUWNjQ0ZiNURCRWc0SlMwa05pWG8zLWc3MTFWVGd2Z284WDMyMS03NWw5MnN6UWpDeDQ3aDFzY243ZmE1TkJhTkdfanVPZjV0QnhFbl9uY3N1TjR3RVRnT0JJVHFVN0xZWmxTVEtUX2lYODFncUJOOWtuWGMtQ0NVZUh1LThvLUdmekh1Y1BsSEFoYXV0aERhdGFYxXSm6pITyZwvdLIkkrMgz0AmKpTBqVCgOX8pJQtghB7wRQAAAAC5P9lh8uZGL7EiggAiR954AEEBSJVTcyTe4miZ8dwly7pJzBQdHKwTZ7oiBpM0DNDfhM_Q4-J-LYuAYP_mHPFGE59BMHV9bqTrcLy2T4zDLCk1UqUBAgMmIAEhWCC0eleNTLgwWxaVBqV139T6hONseRz7HgXRIVS9bPxIjSJYIJ1MfwUhvkSEjeiNJ6y5-w8PuuwMAvfgpN7F4Q2EW79v",
                "clientDataJSON":"eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiZGZvLUhscUpwM01MSy1KNVRMeHhtdlhKaWVTM3pHd2RrOUc5SDliUGV6ZyIsIm9yaWdpbiI6Imh0dHBzOlwvXC93ZWJhdXRobi5pbyIsImFuZHJvaWRQYWNrYWdlTmFtZSI6ImNvbS5hbmRyb2lkLmNocm9tZSJ9"}}"#;

        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.io",
            "webauthn.io",
            vec![Url::parse("https://webauthn.io").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal: HumanBinaryData =
            serde_json::from_str("\"dfo+HlqJp3MLK+J5TLxxmvXJieS3zGwdk9G9H9bPezg=\"").unwrap();
        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(response).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            Some(&(GOOGLE_SAFETYNET_CA_OLD.try_into().unwrap())),
            true,
            &RequestRegistrationExtensions::default(),
            true,
        );
        dbg!(&result);
        assert_eq!(result, Err(WebauthnError::AttestationNotSupported));
    }

    #[test]
    fn test_google_android_key() {
        let chal: HumanBinaryData =
            serde_json::from_str("\"Tf65bS6D5temh2BwvptqgBPb25iZDRxjwC5ans91IIJDrcrOpnWTK4LVgFjeUV4GDMe44w8SI5NsZssIXTUvDg\"").unwrap();

        let response = r#"{
                "rawId": "AZD7huwZVx7aW1efRa6Uq3JTQNorj3qA9yrLINXEcgvCQYtWiSQa1eOIVrXfCmip6MzP8KaITOvRLjy3TUHO7_c",
                "id": "AZD7huwZVx7aW1efRa6Uq3JTQNorj3qA9yrLINXEcgvCQYtWiSQa1eOIVrXfCmip6MzP8KaITOvRLjy3TUHO7_c",
                "response": {
                    "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiVGY2NWJTNkQ1dGVtaDJCd3ZwdHFnQlBiMjVpWkRSeGp3QzVhbnM5MUlJSkRyY3JPcG5XVEs0TFZnRmplVVY0R0RNZTQ0dzhTSTVOc1pzc0lYVFV2RGciLCJvcmlnaW4iOiJodHRwczpcL1wvd2ViYXV0aG4ub3JnIiwiYW5kcm9pZFBhY2thZ2VOYW1lIjoiY29tLmFuZHJvaWQuY2hyb21lIn0",
                    "attestationObject": "o2NmbXRrYW5kcm9pZC1rZXlnYXR0U3RtdKNjYWxnJmNzaWdYRjBEAiAsp6jPtimcSgc-fgIsVwgqRsZX6eU7KKbkVGWa0CRJlgIgH5yuf_laPyNy4PlS6e8ZHjs57iztxGiTqO7G91sdlWBjeDVjg1kCzjCCAsowggJwoAMCAQICAQEwCgYIKoZIzj0EAwIwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRUwEwYDVQQKDAxHb29nbGUsIEluYy4xEDAOBgNVBAsMB0FuZHJvaWQxOzA5BgNVBAMMMkFuZHJvaWQgS2V5c3RvcmUgU29mdHdhcmUgQXR0ZXN0YXRpb24gSW50ZXJtZWRpYXRlMB4XDTE4MTIwMjA5MTAyNVoXDTI4MTIwMjA5MTAyNVowHzEdMBsGA1UEAwwUQW5kcm9pZCBLZXlzdG9yZSBLZXkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQ4SaIP3ibDSwCIORpYJ3g9_5OICxZUCIqt-vV6JZVJoXQ8S1JFzyaFz5EFQ2fNT6-5SE5wWTZRAR_A3M52IcaPo4IBMTCCAS0wCwYDVR0PBAQDAgeAMIH8BgorBgEEAdZ5AgERBIHtMIHqAgECCgEAAgEBCgEBBCAqQ4LXu9idi1vfF3LP7MoUOSSHuf1XHy63K9-X3gbUtgQAMIGCv4MQCAIGAWduLuFwv4MRCAIGAbDqja1wv4MSCAIGAbDqja1wv4U9CAIGAWduLt_ov4VFTgRMMEoxJDAiBB1jb20uZ29vZ2xlLmF0dGVzdGF0aW9uZXhhbXBsZQIBATEiBCBa0F7CIcj4OiJhJ97FV1AMPldLxgElqdwhywvkoAZglTAzoQUxAwIBAqIDAgEDowQCAgEApQUxAwIBBKoDAgEBv4N4AwIBF7-DeQMCAR6_hT4DAgEAMB8GA1UdIwQYMBaAFD_8rNYasTqegSC41SUcxWW7HpGpMAoGCCqGSM49BAMCA0gAMEUCIGd3OQiTgFX9Y07kE-qvwh2Kx6lEG9-Xr2ORT5s7AK_-AiEAucDIlFjCUo4rJfqIxNY93HXhvID7lNzGIolS0E-BJBhZAnwwggJ4MIICHqADAgECAgIQATAKBggqhkjOPQQDAjCBmDELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDU1vdW50YWluIFZpZXcxFTATBgNVBAoMDEdvb2dsZSwgSW5jLjEQMA4GA1UECwwHQW5kcm9pZDEzMDEGA1UEAwwqQW5kcm9pZCBLZXlzdG9yZSBTb2Z0d2FyZSBBdHRlc3RhdGlvbiBSb290MB4XDTE2MDExMTAwNDYwOVoXDTI2MDEwODAwNDYwOVowgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRUwEwYDVQQKDAxHb29nbGUsIEluYy4xEDAOBgNVBAsMB0FuZHJvaWQxOzA5BgNVBAMMMkFuZHJvaWQgS2V5c3RvcmUgU29mdHdhcmUgQXR0ZXN0YXRpb24gSW50ZXJtZWRpYXRlMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE6555-EJjWazLKpFMiYbMcK2QZpOCqXMmE_6sy_ghJ0whdJdKKv6luU1_ZtTgZRBmNbxTt6CjpnFYPts-Ea4QFKNmMGQwHQYDVR0OBBYEFD_8rNYasTqegSC41SUcxWW7HpGpMB8GA1UdIwQYMBaAFMit6XdMRcOjzw0WEOR5QzohWjDPMBIGA1UdEwEB_wQIMAYBAf8CAQAwDgYDVR0PAQH_BAQDAgKEMAoGCCqGSM49BAMCA0gAMEUCIEuKm3vugrzAM4euL8CJmLTdw42rJypFn2kMx8OS1A-OAiEA7toBXbb0MunUhDtiTJQE7zp8zL1e-yK75_65dz9ZP_tZAo8wggKLMIICMqADAgECAgkAogWe0Q5DW1cwCgYIKoZIzj0EAwIwgZgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1Nb3VudGFpbiBWaWV3MRUwEwYDVQQKDAxHb29nbGUsIEluYy4xEDAOBgNVBAsMB0FuZHJvaWQxMzAxBgNVBAMMKkFuZHJvaWQgS2V5c3RvcmUgU29mdHdhcmUgQXR0ZXN0YXRpb24gUm9vdDAeFw0xNjAxMTEwMDQzNTBaFw0zNjAxMDYwMDQzNTBaMIGYMQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNTW91bnRhaW4gVmlldzEVMBMGA1UECgwMR29vZ2xlLCBJbmMuMRAwDgYDVQQLDAdBbmRyb2lkMTMwMQYDVQQDDCpBbmRyb2lkIEtleXN0b3JlIFNvZnR3YXJlIEF0dGVzdGF0aW9uIFJvb3QwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATuXV7H4cDbbQOmfua2G-xNal1qaC4P_39JDn13H0Qibb2xr_oWy8etxXfSVpyqt7AtVAFdPkMrKo7XTuxIdUGko2MwYTAdBgNVHQ4EFgQUyK3pd0xFw6PPDRYQ5HlDOiFaMM8wHwYDVR0jBBgwFoAUyK3pd0xFw6PPDRYQ5HlDOiFaMM8wDwYDVR0TAQH_BAUwAwEB_zAOBgNVHQ8BAf8EBAMCAoQwCgYIKoZIzj0EAwIDRwAwRAIgNSGj74s0Rh6c1WDzHViJIGrco2VB9g2ezooZjGZIYHsCIE0L81HZMHx9W9o1NB2oRxtjpYVlPK1PJKfnTa9BffG_aGF1dGhEYXRhWMWVaQiPHs7jIylUA129ENfK45EwWidRtVm7j9fLsim91EUAAAAAKPN9K5K4QcSwKoYM73zANABBAVUvAmX241vMKYd7ZBdmkNWaYcNYhoSZCJjFRGmROb6I4ygQUVmH6k9IMwcbZGeAQ4v4WMNphORudwje5h7ty9ClAQIDJiABIVggOEmiD94mw0sAiDkaWCd4Pf-TiAsWVAiKrfr1eiWVSaEiWCB0PEtSRc8mhc-RBUNnzU-vuUhOcFk2UQEfwNzOdiHGjw"
                },
                "type": "public-key"}"#;

        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.org",
            "webauthn.org",
            vec![Url::parse("https://webauthn.org").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(response).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Required,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            Some(&(ANDROID_SOFTWARE_ROOT_CA.try_into().unwrap())),
            true,
            &RequestRegistrationExtensions::default(),
            true,
        );
        dbg!(&result);
        assert!(result.is_ok());

        match result.unwrap().attestation.metadata {
            AttestationMetadata::AndroidKey {
                is_km_tee,
                is_attest_tee,
            } => {
                assert!(is_km_tee);
                assert!(!is_attest_tee);
            }
            _ => panic!("invalid metadata"),
        }
    }

    #[test]
    fn test_origins_match_localhost_port() {
        let collected = url::Url::parse("http://localhost:3000").unwrap();
        let config = url::Url::parse("http://localhost:8000").unwrap();

        let result = super::WebauthnCore::origins_match(false, true, &collected, &config);
        dbg!(result);
        assert!(result);

        let result = super::WebauthnCore::origins_match(true, false, &collected, &config);
        assert!(!result);
    }

    #[test]
    fn test_tpm_ecc_aseigler() {
        let chal: HumanBinaryData =
            serde_json::from_str("\"E2YebMmG9992XialpFL1lkPptOIBPeKsphNkt1JcbKk\"").unwrap();

        let response = r#"{
            "id": "BoLAd0jIDI0ztrH1N45XQ_0w_N5ndt3hpNixQi3J2No",
            "rawId": "BoLAd0jIDI0ztrH1N45XQ_0w_N5ndt3hpNixQi3J2No",
            "response": {
              "attestationObject": "o2NmbXRjdHBtZ2F0dFN0bXSmY2FsZzn__mNzaWdZAQAzaz3HmrpCUlkEV2iv-TF2_y0MD7MVc0rLyuD_Ah3X9vx3G21WgeI89PyyvEYw3yEUUdO7sn6YxubMfuePpuSawYKAeSbw3O4LkMDC2fqZmlLyTfoC8L1_8vExv6mWPN7H5U6E_K7IZ38H3mO736ie-mDyoXxalj4WkA9zjKXJM5t7GhHQAqtDaX4HmM47pFH25atgQnoLdB0MTzh6jgYjIiDrMSOqhrQYskiaX_LFfKTiWfviwMOYcMA8FkRPc05LKvPTxp-bx_ghHrd_gIAUA3MjfElVYCVfveMnI61ZwARnf0cTrFp7vfga85YeAXaLOu29JifjodW6DsjL_dnXY3ZlcmMyLjBjeDVjglkFtTCCBbEwggOZoAMCAQICEAaSyUKea0mgpfZbwvZ7byMwDQYJKoZIhvcNAQELBQAwQTE_MD0GA1UEAxM2RVVTLU5UQy1LRVlJRC0yM0Y0RTIyQUQzQkUzNzRBNDQ5NzcyOTU0QUEyODNBRUQ3NTI1NzJFMB4XDTIxMTEyNTIxMzA1NFoXDTI3MDYwMzE3NTE0N1owADCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANwiGFmQdIOYto4qGegANWT-LdSr5T5_tj7E_aKtLSNP8bqc6eP11VvCi9ZFnbjiFxi1NdY2GAbUDb3zr1PnZpOcwvn1gh704PLtkZYFkwvFRvm5bIvtsuqYgn71MCup1GCTeJ3EcylidbVpmwX5s9XK5vyRsMpQ1TxPwxPq32toIBcQ3pgZyb9Ic_m1IfWE_hC_XlwZzqfFnFL7XszCGwJmziFjML9VeBrdv0dkrDWMv1sNI1PDDm_JQ8iZwZ83At3qsgnmwN4zudOMUPRMJBNeiVBj9GjW7tV9tSG2Oa_F_JUo0b1Gr_y08PSMhAckj6ZaR8_EBppoty9CbTm65nsCAwEAAaOCAeQwggHgMA4GA1UdDwEB_wQEAwIHgDAMBgNVHRMBAf8EAjAAMG0GA1UdIAEB_wRjMGEwXwYJKwYBBAGCNxUfMFIwUAYIKwYBBQUHAgIwRB5CAFQAQwBQAEEAIAAgAFQAcgB1AHMAdABlAGQAIAAgAFAAbABhAHQAZgBvAHIAbQAgACAASQBkAGUAbgB0AGkAdAB5MBAGA1UdJQQJMAcGBWeBBQgDMEoGA1UdEQEB_wRAMD6kPDA6MTgwDgYFZ4EFAgMMBWlkOjcyMBAGBWeBBQICDAdOUENUNzV4MBQGBWeBBQIBDAtpZDo0RTU0NDMwMDAfBgNVHSMEGDAWgBTTjd-fy_wwa14b1TQrBpJk2U7fpTAdBgNVHQ4EFgQUeq9wlX_04m4THgx-yMSO7QwViv8wgbIGCCsGAQUFBwEBBIGlMIGiMIGfBggrBgEFBQcwAoaBkmh0dHA6Ly9hemNzcHJvZGV1c2Fpa3B1Ymxpc2guYmxvYi5jb3JlLndpbmRvd3MubmV0L2V1cy1udGMta2V5aWQtMjNmNGUyMmFkM2JlMzc0YTQ0OTc3Mjk1NGFhMjgzYWVkNzUyNTcyZS8xMzY0YTJkMy1hZTU0LTQ3YjktODdmMy0zMjA1NDE5NDc0MGUuY2VyMA0GCSqGSIb3DQEBCwUAA4ICAQCiPgQwqysYPQpMiRDpxbsx24d1xVX_kiUwwcQJE3mSYvwe4tnaQSHjlfB3OkpDMjotxFl33oUMxxScjSrgp_1o6rdkiO6QvPMgsqDMX4w-dmWn00akwNbMasTxg39Ceqtocw4i-R9AlNwndpe3QUIt8xkQ5dhlcIF8lc1dXmgz4mkMAtOi3VgaNvHTsRF9pLbTczJss608X8b4gHqM4t7lfIcRB8DvSyfXc7T3k21-4_3jvAb2HRoCCAyv8_XXn1UwkWTrXMLUSiE1p5Sl8ba8I_86Hsemsc0aflwRZrrY2pC3aaA3QbbfAyskiaFPw-ZibY9p0_QVq1XhAKa-dDd70mWvTGKQdrqfZI_SC5zccvDAm6aefAfnYBY2fV92ZFriihA2ULcJaESz3X3JkiK4eO1k0T2uf9-rL4lUEADibwpnsZOBeNWBsztvXDmcZGR_MSoRIQygKMw2U7AproqBPDRDFwhS5yc9UHvD6dMZ3PLx4i_eo-BLr-QJ2HARoyK8KuV0xLEq3XyjWdfZDbAueUVgtic14wK9jiSbhycRT2WV3-QU8KPm5_QCt_eBPwY81a-q84jm2ue_ok8-LYrmWpvihqRhFhK9MLVS96QaHeeuDehYNDWsSIVCr9jB-lchueZ-kZqwyl_4pPMrM7wLXBOR-bV5_pAPv3u_RvQmhVkG7zCCBuswggTToAMCAQICEzMAAAQHrjuoB9SvW8wAAAAABAcwDQYJKoZIhvcNAQELBQAwgYwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xNjA0BgNVBAMTLU1pY3Jvc29mdCBUUE0gUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgMjAxNDAeFw0yMTA2MDMxNzUxNDdaFw0yNzA2MDMxNzUxNDdaMEExPzA9BgNVBAMTNkVVUy1OVEMtS0VZSUQtMjNGNEUyMkFEM0JFMzc0QTQ0OTc3Mjk1NEFBMjgzQUVENzUyNTcyRTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAMkPU9X8JhPBwDxmFm84D31b8xN5NQz0XR8Nji_-Z8v3WtC4lSdEwJUwqvZkj5OQ3wPA_6haONcCHzqTZhyz1aheOPhXmEeWFWjEiJFj07crEZb9wM4rM1fdcf3vCQNSSDlogC5AM-tITx31hm0YffIrzM3n70fNBBfvlw8t-yhZVOavj7l29gKsyvkR0IadruvLVWWVeH9rueHVrOwlU4wUJpjD41d4U87M3FgUGK2YacQxT0BPHzaOCTE9YhylG5fA_eCF7Q1SxAe347uIaS6I3GhAootzJy9XYeFp_uhc1Yp2hMh5wdeRkm15WKb7tE9T4vwHp0VCQEkUQn1ClN_s7PpfKNFp-DB9ez0Fh7tqag6AssrKE6LgOjfWDWUcgzgIiFLvv9Gx797IZj8LDazK1iGSqI2D8zmmxnGG47MevfY8q2udJW1G4nOcjw49x6XZHmnT3VpVKcTDbI9bEsyc2R9vngftF9FgnEVdyt-QRqE0UqEXJmjLhcxBMeyFZJd_bEAutSBpWugPk10IPFRkXppsuHMZFHJVP96IWwVmm6Q4mX018K996XDubAGblbhvPzJ9NFL_e7xM2ev3rAalz2CzSLYs48EXym7dqGTnP7F9DaF2O0IHT0GQ951wFVoGmA-IYsTMVsdlhVaImCuHgahu1W94H6BvtDkGGku7AgMBAAGjggGOMIIBijAOBgNVHQ8BAf8EBAMCAoQwGwYDVR0lBBQwEgYJKwYBBAGCNxUkBgVngQUIAzAWBgNVHSAEDzANMAsGCSsGAQQBgjcVHzASBgNVHRMBAf8ECDAGAQH_AgEAMB0GA1UdDgQWBBTTjd-fy_wwa14b1TQrBpJk2U7fpTAfBgNVHSMEGDAWgBR6jArOL0hiF-KU0a5VwVLscXSkVjBwBgNVHR8EaTBnMGWgY6Bhhl9odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpb3BzL2NybC9NaWNyb3NvZnQlMjBUUE0lMjBSb290JTIwQ2VydGlmaWNhdGUlMjBBdXRob3JpdHklMjAyMDE0LmNybDB9BggrBgEFBQcBAQRxMG8wbQYIKwYBBQUHMAKGYWh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0JTIwVFBNJTIwUm9vdCUyMENlcnRpZmljYXRlJTIwQXV0aG9yaXR5JTIwMjAxNC5jcnQwDQYJKoZIhvcNAQELBQADggIBAIQJqhFB71eZzZMq0w866QXDKlHcGyIa_IkTK4p5ejIdIA7FJ8neeVToAKUt9ULEb1Od2ir1y5Qx5Zp_edf4F8aikn-yw61hNB3FQ4iSV49eqEMe2Fx6OMBmHRWGtUjAlf5g_N2Qc6rHela2d69nQbpSF3Nq7AESguXxnoqZ-4CGUW0jC_b93sTd5fESHs_iwFX-zWKCwCXerqCuI3PqYWOlbCnftYhsI1CD638wJxw4YFXdSmOrF8dDnd6tlH_0qCZrBX-k4N-8QgK1-BDYIxmvUBnpLFDDitB2dP6YIglY0VcjkPd3BDmodHknG4GQeAvJKHpqF91Y3K1rOWvn4JqzHFvL3JgXgL7LbC_h9EF50HeHayPCToTS8Pmg_4dfUaCwNlxPvu9GvjrDKDNNEV5T73iWMV_GQbVsx6JULAljCthYLo-55mONDcr1x7kakXlQT-yIdIQ57Ix8eHz_qkJkvWxbw8vOgrXhkLK0jGAvW_YSkTV7G9_TYDJ--8IjPPHC1bexKq72-L7KetwH6LbWHGeYkJnaZ1zqeN4USxyJn8K4uhwnjSeK2sZ942zn5EnZnjd85yfdkPLcQY8xtYiWNjc_PprTrjhLyMO71VdMkTDiTTtDha37qywNISPV7vBv8YDiDjX8ElsWbTHTC0XgBp0h-RkjaRKI5C4eTUebZ3B1YkFyZWFYdgAjAAsABAByACCd_8vzbDg65pn7mGjcbcuJ1xU4hL4oA5IsEkFYv60irgAQABAAAwAQACCweOEk52r8mnJ6y9bsGcM3V4dL1LWt8I67Jjx5mcrFuAAgjwd_jaCEEOAJLV97kX3VgbxzopPYMC4NqEFjD0m55PpoY2VydEluZm9Yof9UQ0eAFwAiAAvgBLotxyAAbygBG4efe84V0SVYnO6xLrYaC1oyLgTt3QAUjcjAdORvuzxCfLBU7KNxPFSPE84AAAAUHn9jxccO2yRJARoXARNN0IPNWxnEACIACxfcHNQuRgb_05OKyBrS_1kY5IYxOl67gTlqkHd4g6slACIAC7tcXSHNTw8ANLeZd3PKooKsgrMIlGD47aunn05BcquwaGF1dGhEYXRhWKRqubvw35oW-R27M7uxMvr50Xx4LEgmxuxw7O5Y2X71KkUAAAAACJhwWMrcS4G24TDeUNy-lgAgBoLAd0jIDI0ztrH1N45XQ_0w_N5ndt3hpNixQi3J2NqlAQIDJiABIVggsHjhJOdq_JpyesvW7BnDN1eHS9S1rfCOuyY8eZnKxbgiWCCPB3-NoIQQ4AktX3uRfdWBvHOik9gwLg2oQWMPSbnk-g",
              "clientDataJSON": "eyJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIiwiY2hhbGxlbmdlIjoiRTJZZWJNbUc5OTkyWGlhbHBGTDFsa1BwdE9JQlBlS3NwaE5rdDFKY2JLayIsIm9yaWdpbiI6Imh0dHBzOi8vd2ViYXV0aG4uZmlyc3R5ZWFyLmlkLmF1IiwiY3Jvc3NPcmlnaW4iOmZhbHNlLCJvdGhlcl9rZXlzX2Nhbl9iZV9hZGRlZF9oZXJlIjoiZG8gbm90IGNvbXBhcmUgY2xpZW50RGF0YUpTT04gYWdhaW5zdCBhIHRlbXBsYXRlLiBTZWUgaHR0cHM6Ly9nb28uZ2wveWFiUGV4In0"
            },
            "type": "public-key"}"#;

        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(response).unwrap();

        debug!("{:?}", rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Preferred,
            &chal,
            &[],
            &[COSEAlgorithm::ES256],
            None,
            true,
            &RequestRegistrationExtensions::default(),
            true,
        );

        assert!(matches!(
            result,
            Err(WebauthnError::CredentialInsecureCryptography)
        ))
    }

    #[test]
    fn test_ios_origin_matches() {
        assert!(Webauthn::origins_match(
            false,
            false,
            &Url::parse("ios:bundle-id:com.foo.bar").unwrap(),
            &Url::parse("ios:bundle-id:com.foo.bar").unwrap(),
        ));

        assert!(!Webauthn::origins_match(
            false,
            false,
            &Url::parse("ios:bundle-id:com.foo.bar").unwrap(),
            &Url::parse("ios:bundle-id:com.foo.baz").unwrap(),
        ));
    }

    #[test]
    fn test_solokey_v2_a_sealed_attestation() {
        let chal: HumanBinaryData =
            serde_json::from_str("\"VEP2Y5lrFKvfNZCt-js1BivzIRjDCXERNRswVPGT1tw\"").unwrap();
        let response = r#"{
            "id": "owBYr08K20VJPLwjmm6fiIPE9iqvr31mfxoi1S-gj3mrvsmeSSUd70rMHJpbMBxnm7MlTX8hPpXz2NKVkEVrVGrrJOayYhdthzPeRqPQsFj_f2qkhJrt3xSIzDb6ZzS1hcME5xE76_XKdbH9-ZEUztxN9lR8GjX5TO9e1WsEfeY6yriqKRZ-xgA3BU081GOZWZ00cggWPEEmll1gkYepDDjrwH0a2CXaV-oSs50rRIuD9JkBTKCqEYK6IG-CBMtTEwJQA042FkAQ_RpWpziVVyXfWA",
            "rawId": "owBYr08K20VJPLwjmm6fiIPE9iqvr31mfxoi1S-gj3mrvsmeSSUd70rMHJpbMBxnm7MlTX8hPpXz2NKVkEVrVGrrJOayYhdthzPeRqPQsFj_f2qkhJrt3xSIzDb6ZzS1hcME5xE76_XKdbH9-ZEUztxN9lR8GjX5TO9e1WsEfeY6yriqKRZ-xgA3BU081GOZWZ00cggWPEEmll1gkYepDDjrwH0a2CXaV-oSs50rRIuD9JkBTKCqEYK6IG-CBMtTEwJQA042FkAQ_RpWpziVVyXfWA",
            "response": {
              "attestationObject": "o2NmbXRmcGFja2VkZ2F0dFN0bXSjY2FsZyZjc2lnWEcwRQIhAPdns_NNqPklDOJLgahVz9Ul9yGWelzagMgTc9PSgAliAiAi058w6Dq4C_-44qlEcqoKFldVCGcQxnWh6tL2IXj-mmN4NWOBWQKqMIICpjCCAkygAwIBAgIUfWe3F4mJfmOVopPF8mmAKxBb0igwCgYIKoZIzj0EAwIwLTERMA8GA1UECgwIU29sb0tleXMxCzAJBgNVBAYTAkNIMQswCQYDVQQDDAJGMTAgFw0yMTA1MjMwMDUyMDBaGA8yMDcxMDUxMTAwNTIwMFowgYMxCzAJBgNVBAYTAlVTMREwDwYDVQQKDAhTb2xvS2V5czEiMCAGA1UECwwZQXV0aGVudGljYXRvciBBdHRlc3RhdGlvbjE9MDsGA1UEAww0U29sbyAyIE5GQytVU0ItQSA4NjUyQUJFOUZCRDg0ODEwQTg0MEQ2RkM0NDJBOEMyQyBCMTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABArSyTVT7sDxX0rom6XoIcg8qwMStGV3SjoGRNMqHBSAh2sr4EllUzA1F8yEX5XvUPN_M6DQlqEFGw18UodOjBqjgfAwge0wHQYDVR0OBBYEFBiTdxTWyNCRuzSieBflmHPSJbS1MB8GA1UdIwQYMBaAFEFrtkvvohkN5GJf_SkElrmCKbT4MAkGA1UdEwQCMAAwCwYDVR0PBAQDAgTwMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcwAoYWaHR0cDovL2kuczJwa2kubmV0L2YxLzAnBgNVHR8EIDAeMBygGqAYhhZodHRwOi8vYy5zMnBraS5uZXQvcjEvMCEGCysGAQQBguUcAQEEBBIEEIZSq-n72EgQqEDW_EQqjCwwEwYLKwYBBAGC5RwCAQEEBAMCBDAwCgYIKoZIzj0EAwIDSAAwRQIgMsLnUg5Px2FehxIUNiaey8qeT1FGtlJ1s3LEUGOks-8CIQDNEv5aupDvYxn2iqWSNysv4qpdoqSMytRQ7ctfuJDWN2hhdXRoRGF0YVkBV2q5u_Dfmhb5Hbszu7Ey-vnRfHgsSCbG7HDs7ljZfvUqRQAAAAOGUqvp-9hIEKhA1vxEKowsANOjAFivTwrbRUk8vCOabp-Ig8T2Kq-vfWZ_GiLVL6CPeau-yZ5JJR3vSswcmlswHGebsyVNfyE-lfPY0pWQRWtUausk5rJiF22HM95Go9CwWP9_aqSEmu3fFIjMNvpnNLWFwwTnETvr9cp1sf35kRTO3E32VHwaNflM717VawR95jrKuKopFn7GADcFTTzUY5lZnTRyCBY8QSaWXWCRh6kMOOvAfRrYJdpX6hKznStEi4P0mQFMoKoRgrogb4IEy1MTAlADTjYWQBD9GlanOJVXJd9YowFjT0tQAycgZ0VkMjU1MTkhmCAYTBQYsBg8DBhNGCEY3BgxGDIY6xhdABiiGEoYVhiKGHgYMxgcGOIYdRiiGJMLAhgZGIkYNQkY2A0",
              "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJWRVAyWTVsckZLdmZOWkN0LWpzMUJpdnpJUmpEQ1hFUk5Sc3dWUEdUMXR3Iiwib3JpZ2luIjoiaHR0cHM6Ly93ZWJhdXRobi5maXJzdHllYXIuaWQuYXUiLCJ0eXBlIjoid2ViYXV0aG4uY3JlYXRlIn0",
              "transports": null
            },
            "type": "public-key",
            "extensions": {}
          }"#;

        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "webauthn.firstyear.id.au",
            "webauthn.firstyear.id.au",
            vec![Url::parse("https://webauthn.firstyear.id.au").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(response).unwrap();

        debug!(?rsp_d);

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[COSEAlgorithm::EDDSA],
            None,
            true,
            &RequestRegistrationExtensions::default(),
            true,
        );

        debug!(?result);

        // This is a known fault, solokeys emit invalid attestation with EDDSA
        assert!(matches!(
            result,
            Err(WebauthnError::AttestationStatementSigInvalid)
        ))
    }

    #[test]
    fn test_solokey_v2_a_sealed_ed25519_invalid_cbor() {
        let chal: HumanBinaryData =
            serde_json::from_str("\"KlJqz0evSPAw8cTWpup6SkYJw-RTziV0BBuMH8R-zVM\"").unwrap();

        let response = r#"{
            "id": "owBYn6_Ys3wJqEeCM84k1tMrasG4oPkmzCza-UvzwU5a3V_piE5ZglKlAPMikNcz2LHMMxrlE7CZo6bJZ-QNijw97HdJT8fxky1CW78Yt5yvyYAkPurVqIp0_18ngp3HHu9vL35C7bczMQdJEv3tWjD7XZvzlZlewTiFcSjbnSNROmxxTWUFJM9T8Hsito3g8sDSwc16ogiaPidHoK33fCxVhwFMPCVPuOjlRzLXxUXzAlDXFCg6QebXOL-9KnXq1JsZ",
            "rawId": "owBYn6_Ys3wJqEeCM84k1tMrasG4oPkmzCza-UvzwU5a3V_piE5ZglKlAPMikNcz2LHMMxrlE7CZo6bJZ-QNijw97HdJT8fxky1CW78Yt5yvyYAkPurVqIp0_18ngp3HHu9vL35C7bczMQdJEv3tWjD7XZvzlZlewTiFcSjbnSNROmxxTWUFJM9T8Hsito3g8sDSwc16ogiaPidHoK33fCxVhwFMPCVPuOjlRzLXxUXzAlDXFCg6QebXOL-9KnXq1JsZ",
            "response": {
              "attestationObject": "o2NmbXRkbm9uZWdhdHRTdG10oGhhdXRoRGF0YVkBTEmWDeWIDoxodDQXD2R2YFuP5K65ooYyx5lc87qDHZdjRQAAABMAAAAAAAAAAAAAAAAAAAAAAMOjAFifr9izfAmoR4IzziTW0ytqwbig-SbMLNr5S_PBTlrdX-mITlmCUqUA8yKQ1zPYscwzGuUTsJmjpsln5A2KPD3sd0lPx_GTLUJbvxi3nK_JgCQ-6tWoinT_XyeCncce728vfkLttzMxB0kS_e1aMPtdm_OVmV7BOIVxKNudI1E6bHFNZQUkz1PweyK2jeDywNLBzXqiCJo-J0egrfd8LFWHAUw8JU-46OVHMtfFRfMCUNcUKDpB5tc4v70qderUmxmjAWNPS1ADJyBnRWQyNTUxOSGYIBhtGCEYqxgkGEwYPRhZGKIYaBjWGNoYIRjCEhifGPUYVBj7GDgY0hhNGLQYrxiEGE4VGJkYQxheGJoYUhip",
              "clientDataJSON": "eyJjaGFsbGVuZ2UiOiJLbEpxejBldlNQQXc4Y1RXcHVwNlNrWUp3LVJUemlWMEJCdU1IOFItelZNIiwib3JpZ2luIjoiaHR0cDovL2xvY2FsaG9zdDo4MDgwIiwidHlwZSI6IndlYmF1dGhuLmNyZWF0ZSJ9",
              "transports": null
            },
            "type": "public-key",
            "extensions": {}
        }"#;

        let _ = tracing_subscriber::fmt::try_init();
        let wan = Webauthn::new_unsafe_experts_only(
            "localhost",
            "localhost",
            vec![Url::parse("http://localhost:8080/").unwrap()],
            AUTHENTICATOR_TIMEOUT,
            None,
            None,
        );

        let chal = Challenge::from(chal);

        let rsp_d: RegisterPublicKeyCredential = serde_json::from_str(response).unwrap();

        debug!(?rsp_d);

        let reg_extn = RequestRegistrationExtensions {
            cred_protect: Some(CredProtect {
                credential_protection_policy:
                    CredentialProtectionPolicy::UserVerificationOptionalWithCredentialIDList,
                enforce_credential_protection_policy: Some(false),
            }),
            uvm: Some(true),
            cred_props: Some(true),
            min_pin_length: Some(true),
            hmac_create_secret: Some(true),
        };

        let result = wan.register_credential_internal(
            &rsp_d,
            UserVerificationPolicy::Discouraged_DO_NOT_USE,
            &chal,
            &[],
            &[COSEAlgorithm::EDDSA],
            None,
            true,
            &reg_extn,
            true,
        );

        debug!(?result);

        assert!(matches!(
            result,
            Err(WebauthnError::COSEKeyInvalidCBORValue)
        ))
    }
}
