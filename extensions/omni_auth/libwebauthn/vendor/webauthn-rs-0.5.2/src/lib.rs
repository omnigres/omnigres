//! # Webauthn-rs - Webauthn for Rust Server Applications
//!
//! Webauthn is a standard allowing communication between servers, browsers and authenticators
//! to allow strong, passwordless, cryptographic authentication to be performed. Webauthn
//! is able to operate with many authenticator types, such as U2F, TouchID, Windows Hello
//! and many more.
//!
//! This library aims to provide a secure Webauthn implementation that you can
//! plug into your application, so that you can provide Webauthn to your users.
//!
//! There are a number of focused use cases that this library provides, which are described in
//! the [WebauthnBuilder] and [Webauthn] struct.
//!
//! # Getting started
//!
//! In the simplest case where you just want to replace passwords with strong self contained multifactor
//! authentication, you should use our passkey flow.
//!
//! Remember, no other authentication factors are needed. A passkey combines inbuilt user
//! verification (pin, biometrics, etc) with a hardware cryptographic authenticator.
//!
//! ```
//! use webauthn_rs::prelude::*;
//!
//! let rp_id = "example.com";
//! let rp_origin = Url::parse("https://idm.example.com")
//!     .expect("Invalid URL");
//! let mut builder = WebauthnBuilder::new(rp_id, &rp_origin)
//!     .expect("Invalid configuration");
//! let webauthn = builder.build()
//!     .expect("Invalid configuration");
//!
//! // Initiate a basic registration flow to enroll a cryptographic authenticator
//! let (ccr, skr) = webauthn
//!     .start_passkey_registration(
//!         Uuid::new_v4(),
//!         "claire",
//!         "Claire",
//!         None,
//!     )
//!     .expect("Failed to start registration.");
//! ```
//!
//! After this point you then need to use `finish_passkey_registration`, followed by
//! `start_passkey_authentication` and `finish_passkey_authentication`
//!
//! # Tutorial
//!
//! Tutorials and examples on how to use this library in your website project is on the project
//! github <https://github.com/kanidm/webauthn-rs/tree/master/tutorial>
//!
//! # What is a "Passkey"?
//!
//! Like all good things - "it depends". Mostly it depends who you ask, and at what time they adopted
//! the terminology. There are at least four definitions that we are aware of. A passkey is:
//!
//! * any possible webauthn authenticator - security key, tpm, touch id, etc
//! * a platform authenticator - built into a device such as touch id, tpm, etc. This excludes security keys.
//! * a synchronised credential - backed by a cloud keychain like Apple iCloud or Bitwarden.
//! * a resident key (RK) - a stored, discoverable credential allowing usernameless flows
//!
//! Each of these definitions have different pros and cons, and different usability implications. For
//! example, passkeys as resident keys means you can accidentally brick many ctap2.0 devices by exhausting
//! their storage (forcing them to require a full reset, wiping all credentials). Passkeys as platform
//! authenticators means only certain laptops and phones can use them.
//! Passkeys as synced credentials means only certain devices with specific browser combinations
//! or extensions can use passkeys.
//!
//! In this library we chose to define passkeys as "any possible authenticator". This definition
//! aligns with the W3C WebAuthn Working Group's mission of enabling strong authentication without
//! compromising end-user experience, regardless of their authenticator's modality.
//!
//! We may look to enable (but not require) usernameless flows in the future for on devices which
//! opportunistically create resident keys (such as Apple's iCloud Keychain). However, the platform
//! and browser user experience is not good enough to justify enabling these flows at present.
//!
//! # Features
//!
//! This library supports some optional features that you may wish to use. These are all
//! disabled by default as they have risks associated that you need to be aware of as an
//! authentication provider.
//!
//! ## Allow Serialising Registration and Authentication State
//!
//! During a webauthn registration or authentication ceremony, a random challenge is produced and
//! provided to the client. The full content of what is needed for the server to validate this
//! challenge is stored in the associated registration or authentication state types. This value
//! *MUST* be persisted on the server. If you store this in a cookie or some other form of client
//! side stored value, the client can replay a previous authentication state and signature without
//! possession of, or interaction with the authenticator, bypassing pretty much all of the security guarantees
//! of webauthn. Because of this risk by default these states are *not* allowed to be serialised
//! which prevents them from accidentally being placed into a cookie.
//!
//! However there are some *safe* cases of serialising these values. This includes serialising to
//! a database, or using a cookie "memory store" where the client side cookie is a key into a server-side
//! map or similar. Any of these prevent the replay attack threat.
//!
//! An alternate but "less good" method to mitigate replay attacks is to associate a very short
//! expiry window to the cookie if you need full client side state, but this may still allow some
//! forms of real time replay attacks to occur. We do not recommend this.
//!
//! Enabling the feature `danger-allow-state-serialisation` allows you to re-enable serialisation
//! of these types, provided you accept and understand the handling risks associated.
//!
//! ## Credential Internals and Type Changes
//!
//! By default the type wrappers around the keys are opaque. However in some cases you
//! may wish to migrate a key between types (security key to passkey, attested_passkey to passkey)
//! for example. Alternately, you may wish to access the internals of a credential to implement
//! an alternate serialisation or storage mechanism. In these cases you can access the underlying
//! [Credential] type via Into and From by enabling the feature `danger-credential-internals`. The
//! [Credential] type is exposed via the [prelude] when this feature is enabled.
//!
//! However, you should be aware that manipulating the internals of a [Credential] may affect the usage
//! any security properties of that [Credential] in certain cases. You should be careful when
//! enabling this feature that you do not change internal [Credential] values without understanding
//! the implications.
//!
//! ## User-Presence only SecurityKeys
//!
//! By default, SecurityKeys will opportunistically enforce User Verification (Such as a PIN or
//! Biometric). This prevent UV bypass attacks and allows upgrade of the SecurityKey to a Passkey.
//!
//! Enabling the feature `danger-user-presence-only-security-keys` changes these keys to prevent
//! User Verification if possible. However, newer keys will confusingly force a User Verification
//! on registration, but will then not prompt for this during usage. Some user surveys have shown
//! this to confuse users to why the UV is not requested, and it can lower trust in these tokens
//! when they are elevated to be self-contained MFA (passkey) as the user believes these UV prompts to be
//! unreliable and not verified correctly - in other words it trains users to believe that these
//! prompts do nothing and have no effect. In these cases you MUST communicate to the user that
//! the UV *may* occur on registration and then will not occur again, and that is *by design*.
//!
//! If in doubt, do not enable this feature.
//!
//! ## 'Google Passkey stored in Google Password Manager' Specific Workarounds
//!
//! Android (with GMS Core) has a number of issues in the dialogs they present to users for authenticator
//! selection. Instead of allowing the user to choose what kind of passkey they want to
//! use and create (security key, device screen unlock or 'Google Passkey stored in Google Password
//! Manager'), Android expects every website to implement their own selection UI's ahead of time
//! so that the RP sends the specific options to trigger each of these flows. This adds complexity
//! to RP implementations and a large surface area for mistakes, confusion and inconsistent
//! workflows.
//!
//! By default for maximum compatibility and the most accessible user experience this library
//! sends the options to trigger security keys and the device screen unlock as choices. As RPs
//! must provide methods to allow users to enroll multiple independent devices, we consider that
//! this is a reasonable trade since we allow the widest possible sets of authenticators and Android
//! devices (including devices without GMS Core) to operate.
//!
//! To enable the registration call that triggers the 'Google Passkey stored in Google Password
//! Manager' key flow, you can enable the feature `workaround-google-passkey-specific-issues`. This
//! flow can only be used on Android devices with GMS Core, and you must have a way to detect this
//! ahead of time. This flow must NEVER be triggered on any other class of device or browser.
//!
//! ## Conditional UI / Username Autocompletion
//!
//! Some passkey devices will create a resident key opportunistically during registration. These
//! keys in some cases allow the device to autocomplete the username *and* authenticate in a
//! single step.
//!
//! Not all devices support this, nor do all browsers. As a result you must always support the
//! full passkey flow, and conditional-ui is only opportunistic in itself.
//!
//! User testing has shown that these conditional UI flows in most browsers are hard to activate
//! and may be confusing to users, as they attempt to force users to use caBLE/hybrid. We don't
//! recommend conditional UI as a result.
//!
//! If you still wish to experiment with conditional UI, then enabling the feature `conditional-ui`
//! will expose the needed methods to create conditional-ui mediated challenges and expose the
//! functions to extract the users uuid from the authentication request.

#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![deny(warnings)]
#![warn(unused_extern_crates)]
#![warn(missing_docs)]
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
extern crate tracing;

mod interface;

use std::time::Duration;
use url::Url;
use uuid::Uuid;
use webauthn_rs_core::error::{WebauthnError, WebauthnResult};
use webauthn_rs_core::proto::*;
use webauthn_rs_core::WebauthnCore;

use crate::interface::*;

/// Fake `CredentialID` generator. See [WebauthnFakeCredentialGenerator](fake::WebauthnFakeCredentialGenerator) for more details.
pub mod fake {
    pub use webauthn_rs_core::fake::*;
}

/// A prelude of types that are used by `Webauthn`
pub mod prelude {
    pub use crate::interface::*;
    pub use crate::{Webauthn, WebauthnBuilder};
    pub use base64urlsafedata::Base64UrlSafeData;
    pub use url::Url;
    pub use uuid::Uuid;
    pub use webauthn_rs_core::error::{WebauthnError, WebauthnResult};
    #[cfg(feature = "danger-credential-internals")]
    pub use webauthn_rs_core::proto::Credential;
    pub use webauthn_rs_core::proto::{
        AttestationCa, AttestationCaList, AttestationCaListBuilder, AttestationFormat,
        AuthenticatorAttachment,
    };
    pub use webauthn_rs_core::proto::{
        AttestationMetadata, AuthenticationResult, AuthenticationState, CreationChallengeResponse,
        CredentialID, ParsedAttestation, ParsedAttestationData, PublicKeyCredential,
        RegisterPublicKeyCredential, RequestChallengeResponse,
    };
    pub use webauthn_rs_core::proto::{
        COSEAlgorithm, COSEEC2Key, COSEKey, COSEKeyType, COSEKeyTypeId, COSEOKPKey, COSERSAKey,
        ECDSACurve, EDDSACurve,
    };
}

/// The [Webauthn recommended authenticator interaction timeout][0].
///
/// [0]: https://www.w3.org/TR/webauthn-3/#ref-for-dom-publickeycredentialcreationoptions-timeout
pub const DEFAULT_AUTHENTICATOR_TIMEOUT: Duration = Duration::from_secs(300);

/// A constructor for a new [Webauthn] instance. This accepts and configures a number of site-wide
/// properties that apply to all webauthn operations of this service.
#[derive(Debug)]
pub struct WebauthnBuilder<'a> {
    rp_name: Option<&'a str>,
    rp_id: &'a str,
    allowed_origins: Vec<Url>,
    allow_subdomains: bool,
    allow_any_port: bool,
    timeout: Duration,
    algorithms: Vec<COSEAlgorithm>,
    user_presence_only_security_keys: bool,
}

impl<'a> WebauthnBuilder<'a> {
    /// Initiate a new builder. This takes the relying party id and relying party origin.
    ///
    /// # Safety
    ///
    /// rp_id is what Credentials (Authenticators) bind themselves to - rp_id can NOT be changed
    /// without breaking all of your users' associated credentials in the future!
    ///
    /// # Examples
    ///
    /// ```
    /// use webauthn_rs::prelude::*;
    ///
    /// let rp_id = "example.com";
    /// let rp_origin = Url::parse("https://idm.example.com")
    ///     .expect("Invalid URL");
    /// let mut builder = WebauthnBuilder::new(rp_id, &rp_origin)
    ///     .expect("Invalid configuration");
    /// ```
    ///
    /// # Errors
    ///
    /// rp_id *must* be an effective domain of rp_origin. This means that if you are hosting
    /// `https://idm.example.com`, rp_id must be `idm.example.com`, `example.com` or `com`.
    ///
    /// ```
    /// use webauthn_rs::prelude::*;
    ///
    /// let rp_id = "example.com";
    /// let rp_origin = Url::parse("https://idm.different.com")
    ///     .expect("Invalid URL");
    /// assert!(WebauthnBuilder::new(rp_id, &rp_origin).is_err());
    /// ```
    pub fn new(rp_id: &'a str, rp_origin: &'a Url) -> WebauthnResult<Self> {
        // Check the rp_name and rp_id.
        let valid = rp_origin
            .domain()
            .map(|effective_domain| {
                // We need to prepend the '.' here to ensure that myexample.com != example.com,
                // rather than just ends with.
                effective_domain.ends_with(&format!(".{rp_id}")) || effective_domain == rp_id
            })
            .unwrap_or(false);

        if valid {
            Ok(WebauthnBuilder {
                rp_name: None,
                rp_id,
                allowed_origins: vec![rp_origin.to_owned()],
                allow_subdomains: false,
                allow_any_port: false,
                timeout: DEFAULT_AUTHENTICATOR_TIMEOUT,
                algorithms: COSEAlgorithm::secure_algs(),
                user_presence_only_security_keys: false,
            })
        } else {
            error!("rp_id is not an effective_domain of rp_origin");
            Err(WebauthnError::Configuration)
        }
    }

    /// Setting this flag to true allows subdomains to be considered valid in Webauthn operations.
    /// An example of this is if you wish for `https://au.idm.example.com` to be a valid domain
    /// for Webauthn when the configuration is `https://idm.example.com`. Generally this occurs
    /// when you have a centralised IDM system, but location specific systems with DNS based
    /// redirection or routing.
    ///
    /// If in doubt, do NOT change this value. Defaults to "false".
    pub fn allow_subdomains(mut self, allow: bool) -> Self {
        self.allow_subdomains = allow;
        self
    }

    /// Setting this flag skips port checks on origin matches
    pub fn allow_any_port(mut self, allow: bool) -> Self {
        self.allow_any_port = allow;
        self
    }

    /// Set extra origins to be considered valid in Webauthn operations. A common example of this is
    /// enabling use with iOS or Android native "webauthn-like" APIs, which return different
    /// app-specific origins than a web browser would.
    pub fn append_allowed_origin(mut self, origin: &Url) -> Self {
        self.allowed_origins.push(origin.to_owned());
        self
    }

    /// Set the timeout value to use for credential creation and authentication challenges.
    ///
    /// If not set, this defaults to [`DEFAULT_AUTHENTICATOR_TIMEOUT`], per
    /// [Webauthn Level 3 recommendations][0].
    ///
    /// Short timeouts are difficult for some users to meet, particularly if
    /// they need to physically locate and plug in their authenticator, use a
    /// [hybrid authenticator][1], need to enter a PIN and/or use a fingerprint
    /// reader.
    ///
    /// This may take even longer for users with cognitive, motor, mobility
    /// and/or vision impairments. Even a minor skin condition can make it hard
    /// to use a fingerprint reader!
    ///
    /// Consult the [Webauthn specification's accessibility considerations][2],
    /// [WCAG 2.1's "Enough time" guideline][3] and
    /// ["Timeouts" success criterion][4] when choosing a value, particularly if
    /// it is *shorter* than the default.
    ///
    /// [0]: https://www.w3.org/TR/webauthn-3/#ref-for-dom-publickeycredentialcreationoptions-timeout
    /// [1]: https://www.w3.org/TR/webauthn-3/#dom-authenticatortransport-hybrid
    /// [2]: https://www.w3.org/TR/webauthn-3/#sctn-accessiblility-considerations
    /// [3]: https://www.w3.org/TR/WCAG21/#enough-time
    /// [4]: https://www.w3.org/WAI/WCAG21/Understanding/timeouts.html
    pub fn timeout(mut self, timeout: Duration) -> Self {
        self.timeout = timeout;
        self
    }

    /// Set the relying party name. This may be shown to the user. This value can be changed in
    /// the future without affecting credentials that have already registered.
    ///
    /// If not set, defaults to rp_id.
    pub fn rp_name(mut self, rp_name: &'a str) -> Self {
        self.rp_name = Some(rp_name);
        self
    }

    /// Enable security keys to only require user presence, rather than enforcing
    /// their user-verification state.
    ///
    /// *requires feature danger-user-presence-only-security-keys*
    #[cfg(feature = "danger-user-presence-only-security-keys")]
    pub fn danger_set_user_presence_only_security_keys(mut self, enable: bool) -> Self {
        self.user_presence_only_security_keys = enable;
        self
    }

    /// Complete the construction of the [Webauthn] instance. If an invalid configuration setting
    /// is found, an Error will be returned.
    ///
    /// # Examples
    ///
    /// ```
    /// use webauthn_rs::prelude::*;
    ///
    /// let rp_id = "example.com";
    /// let rp_origin = Url::parse("https://idm.example.com")
    ///     .expect("Invalid URL");
    /// let mut builder = WebauthnBuilder::new(rp_id, &rp_origin)
    ///     .expect("Invalid configuration");
    /// let webauthn = builder.build()
    ///     .expect("Invalid configuration");
    /// ```
    pub fn build(self) -> WebauthnResult<Webauthn> {
        Ok(Webauthn {
            core: WebauthnCore::new_unsafe_experts_only(
                self.rp_name.unwrap_or(self.rp_id),
                self.rp_id,
                self.allowed_origins,
                self.timeout,
                Some(self.allow_subdomains),
                Some(self.allow_any_port),
            ),
            algorithms: self.algorithms,
            user_presence_only_security_keys: self.user_presence_only_security_keys,
        })
    }
}

/// An instance of a Webauthn site. This is the main point of interaction for registering and
/// authenticating credentials for users. Depending on your needs, you'll want to allow users
/// to register and authenticate with different kinds of authenticators.
///
/// __I just want to replace passwords with strong cryptographic authentication, and I don't have other requirements__
///
/// > You should use [`start_passkey_registration`](Webauthn::start_passkey_registration)
///
///
/// __I want to replace passwords with strong multi-factor cryptographic authentication, limited to
/// a known set of controlled and trusted authenticator types__
///
/// This type requires `attestation` enabled as the current form of the Attestation CA List
/// may change in the future.
///
/// > You should use [`start_attested_passkey_registration`](Webauthn::start_attested_passkey_registration)
///
///
/// __I want users to have their identities stored on their devices, and for them to authenticate with
/// strong multi-factor cryptographic authentication limited to a known set of trusted authenticator types__
///
/// This authenticator type consumes limited storage space on users' authenticators, and may result in failures or device
/// bricking.
/// You **MUST** only use it in tightly controlled environments where you supply devices to your
/// users.
///
/// > You should use [`start_attested_resident_key_registration`](Webauthn::start_attested_resident_key_registration) (still in development, requires `resident-key-support` feature)
///
///
/// __I want a security token along with an external password to create multi-factor authentication__
///
/// If possible, consider [`start_passkey_registration`](Webauthn::start_passkey_registration) OR
/// [`start_attested_passkey_registration`](Webauthn::start_attested_passkey_registration)
/// instead - it's likely to provide a better user experience over security keys as MFA!
///
/// > If you really want a security key, you should use [`start_securitykey_registration`](Webauthn::start_securitykey_registration)
///
#[derive(Debug, Clone)]
pub struct Webauthn {
    core: WebauthnCore,
    algorithms: Vec<COSEAlgorithm>,
    user_presence_only_security_keys: bool,
}

impl Webauthn {
    /// Get the currently configured origins
    pub fn get_allowed_origins(&self) -> &[Url] {
        self.core.get_allowed_origins()
    }

    /// Initiate the registration of a new passkey for a user. A passkey is any cryptographic
    /// authenticator which internally verifies the user's identity. As these are self contained
    /// multifactor authenticators, they are far stronger than a password or email-reset link. Due
    /// to how webauthn is designed these authentications are unable to be phished.
    ///
    /// Some examples of passkeys include Yubikeys, TouchID, FaceID, Windows Hello and others.
    ///
    /// The keys *may* exist and 'roam' between multiple devices. For example, Apple allows Passkeys
    /// to sync between devices owned by the same Apple account. This can affect your risk model
    /// related to these credentials, but generally in most cases passkeys are better than passwords!
    ///
    /// You *should* NOT pair this authentication with any other factor. A passkey will always
    /// enforce user-verification (MFA) removing the need for other factors.
    ///
    /// `user_unique_id` *may* be stored in the authenticator. This may allow the credential to
    ///  identify the user during certain client side work flows.
    ///
    /// `user_name` and `user_display_name` *may* be stored in the authenticator. `user_name` is a
    /// friendly account name such as "claire@example.com". `user_display_name` is the persons chosen
    /// way to be identified such as "Claire". Both can change at *any* time on the client side, and
    /// MUST NOT be used as primary keys. They *are not* present in authentication, these are only
    /// present to allow client facing work flows to display human friendly identifiers.
    ///
    /// `exclude_credentials` ensures that a set of credentials may not participate in this registration.
    /// You *should* provide the list of credentials that are already registered to this user's account
    /// to prevent duplicate credential registrations. These credentials *can* be from different
    /// authenticator classes since we only require the `CredentialID`
    ///
    /// # Returns
    ///
    /// This function returns a `CreationChallengeResponse` which you must serialise to json and
    /// send to the user agent (e.g. a browser) for it to conduct the registration. You must persist
    /// on the server the `PasskeyRegistration` which contains the state of this registration
    /// attempt and is paired to the `CreationChallengeResponse`.
    ///
    /// Finally you need to call [`finish_passkey_registration`](Webauthn::finish_passkey_registration)
    /// to complete the registration.
    ///
    /// WARNING ⚠️  YOU MUST STORE THE [PasskeyRegistration] VALUE SERVER SIDE.
    ///
    /// Failure to do so *may* open you to replay attacks which can significantly weaken the
    /// security of this system.
    ///
    /// ```
    /// # use webauthn_rs::prelude::*;
    /// # let rp_id = "example.com";
    /// # let rp_origin = Url::parse("https://idm.example.com")
    /// #     .expect("Invalid URL");
    /// # let mut builder = WebauthnBuilder::new(rp_id, &rp_origin)
    /// #     .expect("Invalid configuration");
    /// # let webauthn = builder.build()
    /// #     .expect("Invalid configuration");
    /// #
    /// // you must store this user's unique id with the account. Alternatelly you can
    /// // use an existed UUID source.
    /// let user_unique_id = Uuid::new_v4();
    ///
    /// // Initiate a basic registration flow, allowing any cryptograhpic authenticator to proceed.
    /// let (ccr, skr) = webauthn
    ///     .start_passkey_registration(
    ///         user_unique_id,
    ///         "claire",
    ///         "Claire",
    ///         None, // No other credentials are registered yet.
    ///     )
    ///     .expect("Failed to start registration.");
    /// ```
    pub fn start_passkey_registration(
        &self,
        user_unique_id: Uuid,
        user_name: &str,
        user_display_name: &str,
        exclude_credentials: Option<Vec<CredentialID>>,
    ) -> WebauthnResult<(CreationChallengeResponse, PasskeyRegistration)> {
        let extensions = Some(RequestRegistrationExtensions {
            cred_protect: Some(CredProtect {
                // Since this may contain PII, we want to enforce this. We also
                // want the device to strictly enforce its UV state.
                credential_protection_policy: CredentialProtectionPolicy::UserVerificationRequired,
                // If set to true, causes many authenticators to shit the bed. We have to just hope
                // and pray instead. This is because many device classes when they see this extension
                // and can't satisfy it, they fail the operation instead.
                enforce_credential_protection_policy: Some(false),
            }),
            uvm: Some(true),
            cred_props: Some(true),
            min_pin_length: None,
            hmac_create_secret: None,
        });

        let builder = self
            .core
            .new_challenge_register_builder(
                user_unique_id.as_bytes(),
                user_name,
                user_display_name,
            )?
            .attestation(AttestationConveyancePreference::None)
            .credential_algorithms(self.algorithms.clone())
            .require_resident_key(false)
            .authenticator_attachment(None)
            .user_verification_policy(UserVerificationPolicy::Required)
            .reject_synchronised_authenticators(false)
            .exclude_credentials(exclude_credentials)
            .hints(None)
            .extensions(extensions);

        self.core
            .generate_challenge_register(builder)
            .map(|(ccr, rs)| (ccr, PasskeyRegistration { rs }))
    }

    /// Initiate the registration of a 'Google Passkey stored in Google Password Manager' on an
    /// Android device with GMS Core.
    ///
    /// This function is required as Android's support for Webauthn/Passkeys is broken
    /// and does not correctly perform authenticator selection for the user. Instead
    /// of Android correctly presenting the choice to users to select between a
    /// security key, or a 'Google Passkey stored in Google Password Manager', Android
    /// expects the Relying Party to pre-select this and send a correct set of options for either
    /// a security key *or* a 'Google Passkey stored in Google Password Manager'.
    ///
    /// If you choose to use this function you *MUST* ensure that the device you are
    /// contacting is an Android device with GMS Core, and you *MUST* provide the user the choice
    /// on your site ahead of time to choose between a security key / screen unlock
    /// (triggered by [`start_passkey_registration`](Webauthn::start_passkey_registration))
    /// or a 'Google Passkey stored in Google Password Manager' (triggered by this function).
    #[cfg(any(
        all(doc, not(doctest)),
        feature = "workaround-google-passkey-specific-issues"
    ))]
    pub fn start_google_passkey_in_google_password_manager_only_registration(
        &self,
        user_unique_id: Uuid,
        user_name: &str,
        user_display_name: &str,
        exclude_credentials: Option<Vec<CredentialID>>,
    ) -> WebauthnResult<(CreationChallengeResponse, PasskeyRegistration)> {
        let extensions = Some(RequestRegistrationExtensions {
            // Android doesn't support cred protect.
            cred_protect: None,
            uvm: Some(true),
            cred_props: Some(true),
            min_pin_length: None,
            hmac_create_secret: None,
        });

        let builder = self
            .core
            .new_challenge_register_builder(
                user_unique_id.as_bytes(),
                user_name,
                user_display_name,
            )?
            .attestation(AttestationConveyancePreference::None)
            .credential_algorithms(self.algorithms.clone())
            // Required for android to work
            .require_resident_key(true)
            // Prevent accidentally triggering RK on security keys
            .authenticator_attachment(Some(AuthenticatorAttachment::Platform))
            .user_verification_policy(UserVerificationPolicy::Required)
            .reject_synchronised_authenticators(false)
            .exclude_credentials(exclude_credentials)
            .hints(Some(vec![PublicKeyCredentialHints::ClientDevice]))
            .extensions(extensions);

        self.core
            .generate_challenge_register(builder)
            .map(|(ccr, rs)| (ccr, PasskeyRegistration { rs }))
    }

    /// Complete the registration of the credential. The user agent (e.g. a browser) will return the data of `RegisterPublicKeyCredential`,
    /// and the server provides its paired [PasskeyRegistration]. The details of the Authenticator
    /// based on the registration parameters are asserted.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    ///
    /// The returned `Passkey` must be associated to the users account, and is used for future
    /// authentications via [`start_passkey_authentication`](Webauthn::start_passkey_authentication).
    ///
    /// You MUST assert that the registered `CredentialID` has not previously been registered.
    /// to any other account.
    pub fn finish_passkey_registration(
        &self,
        reg: &RegisterPublicKeyCredential,
        state: &PasskeyRegistration,
    ) -> WebauthnResult<Passkey> {
        self.core
            .register_credential(reg, &state.rs, None)
            .map(|cred| Passkey { cred })
    }

    /// Given a set of `Passkey`'s, begin an authentication of the user. This returns
    /// a `RequestChallengeResponse`, which should be serialised to json and sent to the user agent (e.g. a browser).
    /// The server must persist the [PasskeyAuthentication] state as it is paired to the
    /// `RequestChallengeResponse` and required to complete the authentication.
    ///
    /// Finally you need to call [`finish_passkey_authentication`](Webauthn::finish_passkey_authentication)
    /// to complete the authentication.
    ///
    /// WARNING ⚠️  YOU MUST STORE THE [PasskeyAuthentication] VALUE SERVER SIDE.
    ///
    /// Failure to do so *may* open you to replay attacks which can significantly weaken the
    /// security of this system.
    pub fn start_passkey_authentication(
        &self,
        creds: &[Passkey],
    ) -> WebauthnResult<(RequestChallengeResponse, PasskeyAuthentication)> {
        let extensions = None;
        let creds = creds.iter().map(|sk| sk.cred.clone()).collect();
        let policy = Some(UserVerificationPolicy::Required);
        let allow_backup_eligible_upgrade = true;
        let hints = None;

        self.core
            .new_challenge_authenticate_builder(creds, policy)
            .map(|builder| {
                builder
                    .extensions(extensions)
                    .allow_backup_eligible_upgrade(allow_backup_eligible_upgrade)
                    .hints(hints)
            })
            .and_then(|b| self.core.generate_challenge_authenticate(b))
            .map(|(rcr, ast)| (rcr, PasskeyAuthentication { ast }))
    }

    /// Given the `PublicKeyCredential` returned by the user agent (e.g. a browser), and the stored [PasskeyAuthentication]
    /// complete the authentication of the user.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    /// On success, [AuthenticationResult] is returned which contains some details of the Authentication
    /// process.
    ///
    /// As per <https://www.w3.org/TR/webauthn-3/#sctn-verifying-assertion> 21:
    ///
    /// If the Credential Counter is greater than 0 you MUST assert that the counter is greater than
    /// the stored counter. If the counter is equal or less than this MAY indicate a cloned credential
    /// and you SHOULD invalidate and reject that credential as a result.
    ///
    /// From this [AuthenticationResult] you *should* update the Credential's Counter value if it is
    /// valid per the above check. If you wish
    /// you *may* use the content of the [AuthenticationResult] for extended validations (such as the
    /// presence of the user verification flag).
    pub fn finish_passkey_authentication(
        &self,
        reg: &PublicKeyCredential,
        state: &PasskeyAuthentication,
    ) -> WebauthnResult<AuthenticationResult> {
        self.core.authenticate_credential(reg, &state.ast)
    }

    /// Initiate the registration of a new security key for a user. A security key is any cryptographic
    /// authenticator acting as a single factor of authentication to supplement a password or some
    /// other authentication factor.
    ///
    /// Some examples of security keys include Yubikeys, Feitian ePass, and others.
    ///
    /// We don't recommend this over [Passkey] or [AttestedPasskey], as today in Webauthn most devices
    /// due to their construction require userVerification to be maintained for user trust. What this
    /// means is that most users will require a password, their security key, and a pin or biometric
    /// on the security key for a total of three factors. This adds friction to the user experience
    /// but is required due to a consistency flaw in CTAP2.0 and newer devices. Since the user already
    /// needs a pin or biometrics, why not just use the device as a self contained MFA?
    ///
    /// You MUST pair this authentication with another factor. A security key may opportunistically
    /// allow and enforce user-verification (MFA), but this is NOT guaranteed.
    ///
    /// `user_unique_id` *may* be stored in the authenticator. This may allow the credential to
    ///  identify the user during certain client side work flows.
    ///
    /// `user_name` and `user_display_name` *may* be stored in the authenticator. `user_name` is a
    /// friendly account name such as "claire@example.com". `user_display_name` is the persons chosen
    /// way to be identified such as "Claire". Both can change at *any* time on the client side, and
    /// MUST NOT be used as primary keys. They *may not* be present in authentication, these are only
    /// present to allow client work flows to display human friendly identifiers.
    ///
    /// `exclude_credentials` ensures that a set of credentials may not participate in this registration.
    /// You *should* provide the list of credentials that are already registered to this user's account
    /// to prevent duplicate credential registrations.
    ///
    /// `attestation_ca_list` contains an optional list of Root CA certificates of authenticator
    /// manufacturers that you wish to trust. For example, if you want to only allow Yubikeys on
    /// your site, then you can provide the Yubico Root CA in this list, to validate that all
    /// registered devices are manufactured by Yubico.
    ///
    /// Extensions may ONLY be accessed if an `attestation_ca_list` is provided, else they
    /// ARE NOT trusted.
    ///
    /// # Returns
    ///
    /// This function returns a `CreationChallengeResponse` which you must serialise to json and
    /// send to the user agent (e.g. a browser) for it to conduct the registration. You must persist
    /// on the server the [SecurityKeyRegistration] which contains the state of this registration
    /// attempt and is paired to the `CreationChallengeResponse`.
    ///
    /// Finally you need to call [`finish_securitykey_registration`](Webauthn::finish_securitykey_registration)
    /// to complete the registration.
    ///
    /// WARNING ⚠️  YOU MUST STORE THE [SecurityKeyRegistration] VALUE SERVER SIDE.
    ///
    /// Failure to do so *may* open you to replay attacks which can significantly weaken the
    /// security of this system.
    ///
    /// ```
    /// # use webauthn_rs::prelude::*;
    ///
    /// # let rp_id = "example.com";
    /// # let rp_origin = Url::parse("https://idm.example.com")
    /// #     .expect("Invalid URL");
    /// # let mut builder = WebauthnBuilder::new(rp_id, &rp_origin)
    /// #     .expect("Invalid configuration");
    /// # let webauthn = builder.build()
    /// #     .expect("Invalid configuration");
    ///
    /// // you must store this user's unique id with the account. Alternatelly you can
    /// // use an existed UUID source.
    /// let user_unique_id = Uuid::new_v4();
    ///
    /// // Initiate a basic registration flow, allowing any cryptograhpic authenticator to proceed.
    /// let (ccr, skr) = webauthn
    ///     .start_securitykey_registration(
    ///         user_unique_id,
    ///         "claire",
    ///         "Claire",
    ///         None,
    ///         None,
    ///         None,
    ///     )
    ///     .expect("Failed to start registration.");
    ///
    /// // Initiate a basic registration flow, hinting that the device is probably roaming (i.e. a usb),
    /// // but it could have any attachement in reality
    /// let (ccr, skr) = webauthn
    ///     .start_securitykey_registration(
    ///         Uuid::new_v4(),
    ///         "claire",
    ///         "Claire",
    ///         None,
    ///         None,
    ///         Some(AuthenticatorAttachment::CrossPlatform),
    ///     )
    ///     .expect("Failed to start registration.");
    ///
    /// // Only allow credentials from manufacturers that are trusted and part of the webauthn-rs
    /// // strict "high quality" list.
    ///
    /// use webauthn_rs_device_catalog::Data;
    /// let device_catalog = Data::strict();
    ///
    /// let attestation_ca_list = (&device_catalog)
    ///     .try_into()
    ///     .expect("Failed to build attestation ca list");
    ///
    /// let (ccr, skr) = webauthn
    ///     .start_securitykey_registration(
    ///         Uuid::new_v4(),
    ///         "claire",
    ///         "Claire",
    ///         None,
    ///         Some(attestation_ca_list),
    ///         None,
    ///     )
    ///     .expect("Failed to start registration.");
    /// ```
    pub fn start_securitykey_registration(
        &self,
        user_unique_id: Uuid,
        user_name: &str,
        user_display_name: &str,
        exclude_credentials: Option<Vec<CredentialID>>,
        attestation_ca_list: Option<AttestationCaList>,
        ui_hint_authenticator_attachment: Option<AuthenticatorAttachment>,
    ) -> WebauthnResult<(CreationChallengeResponse, SecurityKeyRegistration)> {
        let attestation = if let Some(ca_list) = attestation_ca_list.as_ref() {
            if ca_list.is_empty() {
                return Err(WebauthnError::MissingAttestationCaList);
            } else {
                AttestationConveyancePreference::Direct
            }
        } else {
            AttestationConveyancePreference::None
        };

        let cred_protect = if self.user_presence_only_security_keys {
            None
        } else {
            Some(CredProtect {
                // We want the device to strictly enforce its UV state.
                credential_protection_policy: CredentialProtectionPolicy::UserVerificationRequired,
                // If set to true, causes many authenticators to shit the bed. Since this type doesn't
                // have the same strict rules about attestation, then we just use this opportunistically.
                enforce_credential_protection_policy: Some(false),
            })
        };

        let extensions = Some(RequestRegistrationExtensions {
            cred_protect,
            uvm: Some(true),
            cred_props: Some(true),
            min_pin_length: None,
            hmac_create_secret: None,
        });

        let policy = if self.user_presence_only_security_keys {
            UserVerificationPolicy::Discouraged_DO_NOT_USE
        } else {
            UserVerificationPolicy::Preferred
        };

        let builder = self
            .core
            .new_challenge_register_builder(
                user_unique_id.as_bytes(),
                user_name,
                user_display_name,
            )?
            .attestation(attestation)
            .credential_algorithms(self.algorithms.clone())
            .require_resident_key(false)
            .authenticator_attachment(ui_hint_authenticator_attachment)
            .user_verification_policy(policy)
            .reject_synchronised_authenticators(false)
            .exclude_credentials(exclude_credentials)
            .hints(Some(vec![PublicKeyCredentialHints::SecurityKey]))
            .extensions(extensions);

        self.core
            .generate_challenge_register(builder)
            .map(|(ccr, rs)| {
                (
                    ccr,
                    SecurityKeyRegistration {
                        rs,
                        ca_list: attestation_ca_list,
                    },
                )
            })
    }

    /// Complete the registration of the credential. The user agent (e.g. a browser) will return the data of `RegisterPublicKeyCredential`,
    /// and the server provides its paired [SecurityKeyRegistration]. The details of the Authenticator
    /// based on the registration parameters are asserted.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    ///
    /// The returned [SecurityKey] must be associated to the users account, and is used for future
    /// authentications via (`start_securitykey_authentication`)[crate::Webauthn::start_securitykey_authentication].
    ///
    /// You MUST assert that the registered [CredentialID] has not previously been registered.
    /// to any other account.
    ///
    /// # Verifying specific device models
    /// If you wish to assert a specific type of device model is in use, you can inspect the
    /// SecurityKey `attestation()` and its associated metadata. You can use this to check for
    /// specific device aaguids for example.
    ///
    pub fn finish_securitykey_registration(
        &self,
        reg: &RegisterPublicKeyCredential,
        state: &SecurityKeyRegistration,
    ) -> WebauthnResult<SecurityKey> {
        self.core
            .register_credential(reg, &state.rs, state.ca_list.as_ref())
            .map(|cred| SecurityKey { cred })
    }

    /// Given a set of [SecurityKey], begin an authentication of the user. This returns
    /// a `RequestChallengeResponse`, which should be serialised to json and sent to the user agent (e.g. a browser).
    /// The server must persist the [SecurityKeyAuthentication] state as it is paired to the
    /// `RequestChallengeResponse` and required to complete the authentication.
    ///
    /// Finally you need to call [`finish_securitykey_authentication`](Webauthn::finish_securitykey_authentication)
    /// to complete the authentication.
    ///
    /// WARNING ⚠️  YOU MUST STORE THE [SecurityKeyAuthentication] VALUE SERVER SIDE.
    ///
    /// Failure to do so *may* open you to replay attacks which can significantly weaken the
    /// security of this system.
    pub fn start_securitykey_authentication(
        &self,
        creds: &[SecurityKey],
    ) -> WebauthnResult<(RequestChallengeResponse, SecurityKeyAuthentication)> {
        let extensions = None;
        let creds = creds.iter().map(|sk| sk.cred.clone()).collect();
        let allow_backup_eligible_upgrade = false;

        let policy = if self.user_presence_only_security_keys {
            Some(UserVerificationPolicy::Discouraged_DO_NOT_USE)
        } else {
            Some(UserVerificationPolicy::Preferred)
        };

        let hints = Some(vec![PublicKeyCredentialHints::SecurityKey]);

        self.core
            .new_challenge_authenticate_builder(creds, policy)
            .map(|builder| {
                builder
                    .extensions(extensions)
                    .allow_backup_eligible_upgrade(allow_backup_eligible_upgrade)
                    .hints(hints)
            })
            .and_then(|b| self.core.generate_challenge_authenticate(b))
            .map(|(rcr, ast)| (rcr, SecurityKeyAuthentication { ast }))
    }

    /// Given the `PublicKeyCredential` returned by the user agent (e.g. a browser), and the stored [SecurityKeyAuthentication]
    /// complete the authentication of the user.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    /// On success, [AuthenticationResult] is returned which contains some details of the Authentication
    /// process.
    ///
    /// You should use `SecurityKey::update_credential` on the returned [AuthenticationResult] and
    /// ensure it is persisted.
    pub fn finish_securitykey_authentication(
        &self,
        reg: &PublicKeyCredential,
        state: &SecurityKeyAuthentication,
    ) -> WebauthnResult<AuthenticationResult> {
        self.core.authenticate_credential(reg, &state.ast)
    }
}

#[cfg(any(all(doc, not(doctest)), feature = "attestation"))]
impl Webauthn {
    /// Initiate the registration of a new attested_passkey key for a user. A attested_passkey key is a
    /// cryptographic authenticator that is a self-contained multifactor authenticator. This means
    /// that the device (such as a yubikey) verifies the user is who they say they are via a PIN,
    /// biometric or other factor. Only if this verification passes, is the signature released
    /// and provided.
    ///
    /// As a result, the server *only* requires this attested_passkey key to authenticate the user
    /// and assert their identity. Because of this reliance on the authenticator, attestation of
    /// the authenticator and its properties is strongly recommended.
    ///
    /// The primary difference to a passkey, is that these credentials must provide an attestation
    /// certificate which will be cryptographically validated to strictly enforce that only certain
    /// devices may be registered.
    ///
    /// This attestation requires that private key material is bound to a single hardware
    /// authenticator, and cannot be copied or moved out of it. At present, all widely deployed
    /// Hybrid authenticators (Apple iCloud Keychain and Google Passkeys in Google Password
    /// Manager) are synchronised authenticators which can roam between multiple devices, and so can
    /// never be attested.
    ///
    /// As of webauthn-rs v0.5.0, this creates a registration challenge with
    /// [credential selection hints](PublicKeyCredentialHints) that only use ClientDevice or
    /// SecurityKey devices, so a user-agent supporting Webauthn L3 won't offer to use Hybrid
    /// credentials. On user-agents not supporting Webauthn L3, and on older versions of
    /// webauthn-rs, user-agents would show a QR code and a user could attempt to register a
    /// Hybrid authenticator, but it would always fail at the end -- which is a frustrating user
    /// experience!
    ///
    /// You *should* recommend to the user to register multiple attested_passkey keys to their account on
    /// separate devices so that they have fall back authentication in the case of device failure or loss.
    ///
    /// You *should* have a workflow that allows a user to register new devices without a need to register
    /// other factors. For example, allow a QR code that can be scanned from a phone, or a one-time
    /// link that can be copied to the device.
    ///
    /// You *must* have a recovery workflow in case all devices are lost or destroyed.
    ///
    /// `user_unique_id` *may* be stored in the authenticator. This may allow the credential to
    ///  identify the user during certain client side work flows.
    ///
    /// `user_name` and `user_display_name` *may* be stored in the authenticator. `user_name` is a
    /// friendly account name such as "claire@example.com". `user_display_name` is the persons chosen
    /// way to be identified such as "Claire". Both can change at *any* time on the client side, and
    /// MUST NOT be used as primary keys. They *may not* be present in authentication, these are only
    /// present to allow client work flows to display human friendly identifiers.
    ///
    /// `exclude_credentials` ensures that a set of credentials may not participate in this registration.
    /// You *should* provide the list of credentials that are already registered to this user's account
    /// to prevent duplicate credential registrations.
    ///
    /// `attestation_ca_list` contains a required list of Root CA certificates of authenticator
    /// manufacturers that you wish to trust. For example, if you want to only allow Yubikeys on
    /// your site, then you can provide the Yubico Root CA in this list, to validate that all
    /// registered devices are manufactured by Yubico.
    ///
    /// `ui_hint_authenticator_attachment` provides a UX/UI hint to the browser about the types
    /// of credentials that could be used in this registration. If set to `None` all authenticator
    /// attachement classes are valid. If set to Platform, only authenticators that are part of the
    /// device are used such as a TPM or TouchId. If set to Cross-Platform, only devices that are
    /// removable from the device can be used such as yubikeys.
    ///
    /// # Returns
    ///
    /// This function returns a `CreationChallengeResponse` which you must serialise to json and
    /// send to the user agent (e.g. a browser) for it to conduct the registration. You must persist
    /// on the server the `AttestedPasskeyRegistration` which contains the state of this registration
    /// attempt and is paired to the `CreationChallengeResponse`.
    ///
    /// Finally you need to call [`finish_attested_passkey_registration`](Webauthn::finish_attested_passkey_registration)
    /// to complete the registration.
    ///
    /// WARNING ⚠️  YOU MUST STORE THE [AttestedPasskeyRegistration] VALUE SERVER SIDE.
    ///
    /// Failure to do so *may* open you to replay attacks which can significantly weaken the
    /// security of this system.
    ///
    /// ```
    /// # use webauthn_rs::prelude::*;
    /// use webauthn_rs_device_catalog::Data;
    /// # let rp_id = "example.com";
    /// # let rp_origin = Url::parse("https://idm.example.com")
    /// #     .expect("Invalid url");
    /// # let mut builder = WebauthnBuilder::new(rp_id, &rp_origin)
    /// #     .expect("Invalid configuration");
    /// # let webauthn = builder.build()
    /// #     .expect("Invalid configuration");
    /// #
    /// // you must store this user's unique id with the account. Alternatively you can
    /// // use an existed UUID source.
    /// let user_unique_id = Uuid::new_v4();
    ///
    /// // Create a device catalog reference that contains a list of known high quality authenticators
    /// let device_catalog = Data::all_known_devices();
    ///
    /// let attestation_ca_list = (&device_catalog)
    ///     .try_into()
    ///     .expect("Failed to build attestation ca list");
    ///
    /// // Initiate a basic registration flow, allowing any attested cryptograhpic authenticator to proceed.
    /// // Hint (but do not enforce) that we prefer this to be a token/key like a yubikey.
    /// // To enforce this you can validate the properties of the returned device aaguid.
    /// let (ccr, skr) = webauthn
    ///     .start_attested_passkey_registration(
    ///         user_unique_id,
    ///         "claire",
    ///         "Claire",
    ///         None,
    ///         attestation_ca_list,
    ///         Some(AuthenticatorAttachment::CrossPlatform),
    ///     )
    ///     .expect("Failed to start registration.");
    ///
    /// // Only allow credentials from manufacturers that are trusted and part of the webauthn-rs
    /// // strict "high quality" list.
    /// // Hint (but do not enforce) that we prefer this to be a device like TouchID.
    /// // To enforce this you can validate the attestation ca used along with the returned device aaguid
    ///
    /// let device_catalog = Data::strict();
    ///
    /// let attestation_ca_list = (&device_catalog)
    ///     .try_into()
    ///     .expect("Failed to build attestation ca list");
    ///
    /// let (ccr, skr) = webauthn
    ///     .start_attested_passkey_registration(
    ///         Uuid::new_v4(),
    ///         "claire",
    ///         "Claire",
    ///         None,
    ///         attestation_ca_list,
    ///         Some(AuthenticatorAttachment::Platform),
    ///     )
    ///     .expect("Failed to start registration.");
    /// ```
    pub fn start_attested_passkey_registration(
        &self,
        user_unique_id: Uuid,
        user_name: &str,
        user_display_name: &str,
        exclude_credentials: Option<Vec<CredentialID>>,
        attestation_ca_list: AttestationCaList,
        ui_hint_authenticator_attachment: Option<AuthenticatorAttachment>,
        // extensions
    ) -> WebauthnResult<(CreationChallengeResponse, AttestedPasskeyRegistration)> {
        if attestation_ca_list.is_empty() {
            return Err(WebauthnError::MissingAttestationCaList);
        }

        let extensions = Some(RequestRegistrationExtensions {
            cred_protect: Some(CredProtect {
                // Since this may contain PII, we need to enforce this. We also
                // want the device to strictly enforce its UV state.
                credential_protection_policy: CredentialProtectionPolicy::UserVerificationRequired,
                // Set to true since this function requires attestation, and attestation is really
                // only viable on FIDO2/CTAP2 creds that actually support this.
                enforce_credential_protection_policy: Some(true),
            }),
            // https://www.w3.org/TR/webauthn-2/#sctn-uvm-extension
            uvm: Some(true),
            cred_props: Some(true),
            // https://fidoalliance.org/specs/fido-v2.1-rd-20210309/fido-client-to-authenticator-protocol-v2.1-rd-20210309.html#sctn-minpinlength-extension
            min_pin_length: Some(true),
            hmac_create_secret: Some(true),
        });

        let builder = self
            .core
            .new_challenge_register_builder(
                user_unique_id.as_bytes(),
                user_name,
                user_display_name,
            )?
            .attestation(AttestationConveyancePreference::Direct)
            .credential_algorithms(self.algorithms.clone())
            .require_resident_key(false)
            .authenticator_attachment(ui_hint_authenticator_attachment)
            .user_verification_policy(UserVerificationPolicy::Required)
            .reject_synchronised_authenticators(true)
            .exclude_credentials(exclude_credentials)
            .hints(Some(
                // hybrid does NOT perform attestation
                vec![
                    PublicKeyCredentialHints::ClientDevice,
                    PublicKeyCredentialHints::SecurityKey,
                ],
            ))
            .attestation_formats(Some(vec![
                AttestationFormat::Packed,
                AttestationFormat::Tpm,
            ]))
            .extensions(extensions);

        self.core
            .generate_challenge_register(builder)
            .map(|(ccr, rs)| {
                (
                    ccr,
                    AttestedPasskeyRegistration {
                        rs,
                        ca_list: attestation_ca_list,
                    },
                )
            })
    }

    /// Complete the registration of the credential. The user agent (e.g. a browser) will return the data of `RegisterPublicKeyCredential`,
    /// and the server provides its paired [AttestedPasskeyRegistration]. The details of the Authenticator
    /// based on the registration parameters are asserted.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    /// The returned [AttestedPasskey] must be associated to the users account, and is used for future
    /// authentications via [crate::Webauthn::start_attested_passkey_authentication].
    ///
    /// # Verifying specific device models
    /// If you wish to assert a specific type of device model is in use, you can inspect the
    /// AttestedPasskey `attestation()` and its associated metadata. You can use this to check for
    /// specific device aaguids for example.
    ///
    pub fn finish_attested_passkey_registration(
        &self,
        reg: &RegisterPublicKeyCredential,
        state: &AttestedPasskeyRegistration,
    ) -> WebauthnResult<AttestedPasskey> {
        self.core
            .register_credential(reg, &state.rs, Some(&state.ca_list))
            .map(|cred| AttestedPasskey { cred })
    }

    /// Given a set of `AttestedPasskey`'s, begin an authentication of the user. This returns
    /// a `RequestChallengeResponse`, which should be serialised to json and sent to the user agent (e.g. a browser).
    /// The server must persist the [AttestedPasskeyAuthentication] state as it is paired to the
    /// `RequestChallengeResponse` and required to complete the authentication.
    ///
    /// Finally you need to call [`finish_attested_passkey_authentication`](Webauthn::finish_attested_passkey_authentication)
    /// to complete the authentication.
    ///
    /// WARNING ⚠️  YOU MUST STORE THE [AttestedPasskeyAuthentication] VALUE SERVER SIDE.
    ///
    /// Failure to do so *may* open you to replay attacks which can significantly weaken the
    /// security of this system.
    pub fn start_attested_passkey_authentication(
        &self,
        creds: &[AttestedPasskey],
    ) -> WebauthnResult<(RequestChallengeResponse, AttestedPasskeyAuthentication)> {
        let creds = creds.iter().map(|sk| sk.cred.clone()).collect();

        let extensions = Some(RequestAuthenticationExtensions {
            appid: None,
            uvm: Some(true),
            hmac_get_secret: None,
        });

        let policy = Some(UserVerificationPolicy::Required);
        let allow_backup_eligible_upgrade = false;

        let hints = Some(vec![
            PublicKeyCredentialHints::SecurityKey,
            PublicKeyCredentialHints::ClientDevice,
        ]);

        self.core
            .new_challenge_authenticate_builder(creds, policy)
            .map(|builder| {
                builder
                    .extensions(extensions)
                    .allow_backup_eligible_upgrade(allow_backup_eligible_upgrade)
                    .hints(hints)
            })
            .and_then(|b| self.core.generate_challenge_authenticate(b))
            .map(|(rcr, ast)| (rcr, AttestedPasskeyAuthentication { ast }))
    }

    /// Given the `PublicKeyCredential` returned by the user agent (e.g. a browser), and the stored [AttestedPasskeyAuthentication]
    /// complete the authentication of the user. This asserts that user verification must have been correctly
    /// performed allowing you to trust this as a MFA interfaction.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    /// On success, [AuthenticationResult] is returned which contains some details of the Authentication
    /// process.
    ///
    /// As per <https://www.w3.org/TR/webauthn-3/#sctn-verifying-assertion> 21:
    ///
    /// If the Credential Counter is greater than 0 you MUST assert that the counter is greater than
    /// the stored counter. If the counter is equal or less than this MAY indicate a cloned credential
    /// and you SHOULD invalidate and reject that credential as a result.
    ///
    /// From this [AuthenticationResult] you *should* update the Credential's Counter value if it is
    /// valid per the above check. If you wish
    /// you *may* use the content of the [AuthenticationResult] for extended validations (such as the
    /// user verification flag).
    pub fn finish_attested_passkey_authentication(
        &self,
        reg: &PublicKeyCredential,
        state: &AttestedPasskeyAuthentication,
    ) -> WebauthnResult<AuthenticationResult> {
        self.core.authenticate_credential(reg, &state.ast)
    }
}

#[cfg(any(all(doc, not(doctest)), feature = "conditional-ui"))]
impl Webauthn {
    /// This function will initiate a conditional ui authentication for discoverable
    /// credentials.
    ///
    /// Since this relies on the client to "discover" what credential and user id to
    /// use, there are no options required to start this.
    pub fn start_discoverable_authentication(
        &self,
    ) -> WebauthnResult<(RequestChallengeResponse, DiscoverableAuthentication)> {
        let policy = Some(UserVerificationPolicy::Required);
        let extensions = Some(RequestAuthenticationExtensions {
            appid: None,
            uvm: Some(true),
            hmac_get_secret: None,
        });
        let allow_backup_eligible_upgrade = false;
        let hints = None;

        self.core
            .new_challenge_authenticate_builder(Vec::with_capacity(0), policy)
            .map(|builder| {
                builder
                    .extensions(extensions)
                    .allow_backup_eligible_upgrade(allow_backup_eligible_upgrade)
                    .hints(hints)
            })
            .and_then(|b| self.core.generate_challenge_authenticate(b))
            .map(|(mut rcr, ast)| {
                // Force conditional ui - this is not a generic discoverable credential
                // workflow!
                rcr.mediation = Some(Mediation::Conditional);
                (rcr, DiscoverableAuthentication { ast })
            })
    }

    /// Pre-process the clients response from a conditional ui authentication attempt. This
    /// will extract the users Uuid and the Credential ID that was used by the user
    /// in their authentication.
    ///
    /// You must use this information to locate the relavent credential that was used
    /// to allow you to finish the authentication.
    pub fn identify_discoverable_authentication<'a>(
        &'_ self,
        reg: &'a PublicKeyCredential,
    ) -> WebauthnResult<(Uuid, &'a [u8])> {
        let cred_id = reg.get_credential_id();
        reg.get_user_unique_id()
            .and_then(|b| Uuid::from_slice(b).ok())
            .map(|u| (u, cred_id))
            .ok_or(WebauthnError::InvalidUserUniqueId)
    }

    /// Given the `PublicKeyCredential` returned by the user agent (e.g. a browser), the
    /// stored [DiscoverableAuthentication] and the users [DiscoverableKey],
    /// complete the authentication of the user. This asserts that user verification must have been correctly
    /// performed allowing you to trust this as a MFA interfaction.
    ///
    /// # Errors
    /// If any part of the registration is incorrect or invalid, an error will be returned. See [WebauthnError].
    ///
    /// # Returns
    /// On success, [AuthenticationResult] is returned which contains some details of the Authentication
    /// process.
    ///
    /// As per <https://www.w3.org/TR/webauthn-3/#sctn-verifying-assertion> 21:
    ///
    /// If the Credential Counter is greater than 0 you MUST assert that the counter is greater than
    /// the stored counter. If the counter is equal or less than this MAY indicate a cloned credential
    /// and you SHOULD invalidate and reject that credential as a result.
    ///
    /// From this [AuthenticationResult] you *should* update the Credential's Counter value if it is
    /// valid per the above check. If you wish
    /// you *may* use the content of the [AuthenticationResult] for extended validations (such as the
    /// user verification flag).
    pub fn finish_discoverable_authentication(
        &self,
        reg: &PublicKeyCredential,
        mut state: DiscoverableAuthentication,
        creds: &[DiscoverableKey],
    ) -> WebauthnResult<AuthenticationResult> {
        let creds = creds.iter().map(|dk| dk.cred.clone()).collect();
        state.ast.set_allowed_credentials(creds);
        self.core.authenticate_credential(reg, &state.ast)
    }
}

#[cfg(any(all(doc, not(doctest)), feature = "resident-key-support"))]
impl Webauthn {
    /// TODO
    pub fn start_attested_resident_key_registration(
        &self,
        user_unique_id: Uuid,
        user_name: &str,
        user_display_name: &str,
        exclude_credentials: Option<Vec<CredentialID>>,
        attestation_ca_list: AttestationCaList,
        ui_hint_authenticator_attachment: Option<AuthenticatorAttachment>,
    ) -> WebauthnResult<(CreationChallengeResponse, AttestedResidentKeyRegistration)> {
        if attestation_ca_list.is_empty() {
            return Err(WebauthnError::MissingAttestationCaList);
        }

        // credProtect
        let extensions = Some(RequestRegistrationExtensions {
            cred_protect: Some(CredProtect {
                // Since this will contain PII, we need to enforce this.
                credential_protection_policy: CredentialProtectionPolicy::UserVerificationRequired,
                // Set to true since this function requires attestation, and attestation is really
                // only viable on FIDO2/CTAP2 creds that actually support this.
                enforce_credential_protection_policy: Some(true),
            }),
            // https://www.w3.org/TR/webauthn-2/#sctn-uvm-extension
            uvm: Some(true),
            cred_props: Some(true),
            // https://fidoalliance.org/specs/fido-v2.1-rd-20210309/fido-client-to-authenticator-protocol-v2.1-rd-20210309.html#sctn-minpinlength-extension
            min_pin_length: Some(true),
            hmac_create_secret: Some(true),
        });

        let builder = self
            .core
            .new_challenge_register_builder(
                user_unique_id.as_bytes(),
                user_name,
                user_display_name,
            )?
            .attestation(AttestationConveyancePreference::Direct)
            .credential_algorithms(self.algorithms.clone())
            .require_resident_key(true)
            .authenticator_attachment(ui_hint_authenticator_attachment)
            .user_verification_policy(UserVerificationPolicy::Required)
            .reject_synchronised_authenticators(true)
            .exclude_credentials(exclude_credentials)
            .hints(Some(
                // hybrid does NOT perform attestation
                vec![
                    PublicKeyCredentialHints::ClientDevice,
                    PublicKeyCredentialHints::SecurityKey,
                ],
            ))
            .attestation_formats(Some(vec![
                AttestationFormat::Packed,
                AttestationFormat::Tpm,
            ]))
            .extensions(extensions);

        self.core
            .generate_challenge_register(builder)
            .map(|(ccr, rs)| {
                (
                    ccr,
                    AttestedResidentKeyRegistration {
                        rs,
                        ca_list: attestation_ca_list,
                    },
                )
            })
    }

    /// TODO
    pub fn finish_attested_resident_key_registration(
        &self,
        reg: &RegisterPublicKeyCredential,
        state: &AttestedResidentKeyRegistration,
    ) -> WebauthnResult<AttestedResidentKey> {
        let cred = self
            .core
            .register_credential(reg, &state.rs, Some(&state.ca_list))?;

        trace!("finish attested_resident_key -> {:?}", cred);

        // cred protect ignored :(
        // Is the pin long enough?
        // Is it rk?
        // I guess we'll never know ...

        // Is it an approved cred / aaguid?

        Ok(AttestedResidentKey { cred })
    }

    /// TODO
    pub fn start_attested_resident_key_authentication(
        &self,
        creds: &[AttestedResidentKey],
    ) -> WebauthnResult<(RequestChallengeResponse, AttestedResidentKeyAuthentication)> {
        let creds = creds.iter().map(|sk| sk.cred.clone()).collect();
        let extensions = Some(RequestAuthenticationExtensions {
            appid: None,
            uvm: Some(true),
            hmac_get_secret: None,
        });

        let policy = Some(UserVerificationPolicy::Required);
        let allow_backup_eligible_upgrade = false;

        let hints = Some(vec![
            PublicKeyCredentialHints::SecurityKey,
            PublicKeyCredentialHints::ClientDevice,
        ]);

        self.core
            .new_challenge_authenticate_builder(creds, policy)
            .map(|builder| {
                builder
                    .extensions(extensions)
                    .allow_backup_eligible_upgrade(allow_backup_eligible_upgrade)
                    .hints(hints)
            })
            .and_then(|b| self.core.generate_challenge_authenticate(b))
            .map(|(rcr, ast)| (rcr, AttestedResidentKeyAuthentication { ast }))
    }

    /// TODO
    pub fn finish_attested_resident_key_authentication(
        &self,
        reg: &PublicKeyCredential,
        state: &AttestedResidentKeyAuthentication,
    ) -> WebauthnResult<AuthenticationResult> {
        self.core.authenticate_credential(reg, &state.ast)
    }
}

#[test]
/// Test that building a webauthn object from a chrome extension origin is successful.
fn test_webauthnbuilder_chrome_url() -> Result<(), Box<dyn std::error::Error>> {
    use crate::prelude::*;
    let rp_id = "2114c9f524d0cbd74dbe846a51c3e5b34b83ac02c5220ec5cdff751096fa25a5";
    let rp_origin = Url::parse(&format!("chrome-extension://{rp_id}"))?;
    eprintln!("{rp_origin:?}");
    let builder = WebauthnBuilder::new(rp_id, &rp_origin)?;
    eprintln!("rp_id: {:?}", builder.rp_id);
    let built = builder.build()?;
    eprintln!("rp_name: {}", built.core.rp_name());
    Ok(())
}
