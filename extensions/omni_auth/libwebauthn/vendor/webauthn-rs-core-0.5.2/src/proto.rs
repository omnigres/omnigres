//! JSON Protocol Structs and representations for communication with authenticators
//! and clients.

use crate::base64_data::Base64UrlSafeData;
use crate::error::*;
use serde::{Deserialize, Serialize};
use std::{collections::BTreeMap, convert::TryFrom};

#[cfg(feature = "wasm")]
use js_sys::{Array, Object, Uint8Array};
#[cfg(feature = "wasm")]
use wasm_bindgen::prelude::*;

use std::borrow::Borrow;
use std::ops::Deref;


#[cfg(feature = "core")]


impl CredProtect {
    /// Create a [CredProtect] object
    pub fn new(
        credential_protection_policy: CredentialProtectionPolicy,
        enforce_credential_protection_policy: Option<bool>,
    ) -> Self {
        CredProtect {
            credential_protection_policy,
            enforce_credential_protection_policy,
        }
    }
}


impl RequestAuthenticationExtensions {
    /// Get a builder for the [RequestRegistrationExtensions] struct
    #[must_use]
    pub fn builder() -> RequestAuthenticationExtensionsBuilder {
        RequestAuthenticationExtensionsBuilder::new()
    }
}

/// Builder for [RequestAuthenticationExtensions] objects.
pub struct RequestAuthenticationExtensionsBuilder(RequestAuthenticationExtensions);

impl RequestAuthenticationExtensionsBuilder {
    pub(crate) fn new() -> Self {
        Self(RequestAuthenticationExtensions {
            appid: None,
        })
    }

    /// Returns the inner extensions struct
    pub fn build(self) -> RequestAuthenticationExtensions {
        self.0
    }

    /// Set the AppId extension, for backwards compatibility with FIDO U2F credentials
    ///
    /// # Example
    /// ```
    /// # use webauthn_rs_core::proto::RequestAuthenticationExtensions;
    /// let extensions = RequestAuthenticationExtensions::builder()
    ///     .appid(String::from("https://domain.tld/app-id.json"))
    ///     .build();
    ///
    /// assert_eq!(extensions.appid, Some(String::from("https://domain.tld/app-id.json")));
    /// ```
    pub fn appid(mut self, appid: String) -> Self {
        self.0.appid = Some(appid);
        self
    }
}


impl RequestRegistrationExtensions {
    /// Get a builder for the [RequestRegistrationExtensions] struct
    #[must_use]
    pub fn builder() -> RequestRegistrationExtensionsBuilder {
        RequestRegistrationExtensionsBuilder::new()
    }
}

/// Builder for [RequestRegistrationExtensions] objects.
pub struct RequestRegistrationExtensionsBuilder(RequestRegistrationExtensions);

impl RequestRegistrationExtensionsBuilder {
    pub(crate) fn new() -> Self {
        Self(RequestRegistrationExtensions {
            cred_protect: None,
        })
    }

    /// Returns the inner extensions struct
    pub fn build(self) -> RequestRegistrationExtensions {
        self.0
    }

    /// Set the credential protection extension options
    ///
    /// # Example
    /// ```
    /// # use webauthn_rs_core::proto::{RequestRegistrationExtensions, CredentialProtectionPolicy, CredProtect};
    /// let cred_protect = CredProtect::new(
    ///     CredentialProtectionPolicy::UserVerificationRequired,
    ///     None,
    /// );
    /// let extensions = RequestRegistrationExtensions::builder()
    ///     .cred_protect(cred_protect.clone())
    ///     .build();
    ///
    /// assert_eq!(extensions.cred_protect, Some(cred_protect));
    /// ```
    pub fn cred_protect(mut self, cred_protect: CredProtect) -> Self {
        self.0.cred_protect = Some(cred_protect);
        self
    }
}



/// <https://w3c.github.io/webauthn/#sctn-attestation>
#[cfg(feature = "core")]


#[cfg(feature = "core")]

#[cfg(feature = "core")]
#[derive(Debug)]
pub(crate) struct AuthenticatorAssertionResponse<T: Ceremony> {
    pub(crate) authenticator_data: AuthenticatorData<T>,
    pub(crate) authenticator_data_bytes: Vec<u8>,
    pub(crate) client_data: CollectedClientData,
    pub(crate) client_data_bytes: Vec<u8>,
    pub(crate) signature: Vec<u8>,
    pub(crate) _user_handle: Option<Vec<u8>>,
}

#[cfg(feature = "core")]
impl<T: Ceremony> TryFrom<&AuthenticatorAssertionResponseRaw>
    for AuthenticatorAssertionResponse<T>
{
    type Error = WebauthnError;
    fn try_from(aarr: &AuthenticatorAssertionResponseRaw) -> Result<Self, Self::Error> {
        Ok(AuthenticatorAssertionResponse {
            authenticator_data: AuthenticatorData::try_from(aarr.authenticator_data.as_ref())?,
            authenticator_data_bytes: aarr.authenticator_data.clone().into(),
            client_data: CollectedClientData::try_from(aarr.client_data_json.as_ref())?,
            client_data_bytes: aarr.client_data_json.clone().into(),
            signature: aarr.signature.clone().into(),
            _user_handle: aarr.user_handle.clone().map(|uh| uh.into()),
        })
    }
}


/// A client response to an authentication challenge. This contains all required
/// information to asses and assert trust in a credentials legitimacy, followed
/// by authentication to a user.
///
/// You should not need to handle the inner content of this structure - you should
/// provide this to the correctly handling function of Webauthn only.
#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct PublicKeyCredential {
    /// The credential Id, likely base64
    pub id: String,
    /// The binary of the credential id.
    #[serde(rename = "rawId")]
    pub raw_id: Base64UrlSafeData,
    /// The authenticator response.
    pub response: AuthenticatorAssertionResponseRaw,
    /// The extensions sent by the client
    pub extensions: Option<AuthenticationExtensionsClientOutputs>,
    /// The authenticator type.
    #[serde(rename = "type")]
    pub type_: String,
}

impl PublicKeyCredential {
    /// Get the supplied userHandle if provided
    pub fn get_user_handle(&self) -> Option<&[u8]> {
        self.response.user_handle.as_ref().map(|uh| uh.as_ref())
    }
}

