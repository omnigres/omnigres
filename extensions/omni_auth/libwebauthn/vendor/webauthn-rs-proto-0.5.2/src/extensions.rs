//! Extensions allowing certain types of authenticators to provide supplemental information.

use base64urlsafedata::Base64UrlSafeData;
use serde::{Deserialize, Serialize};

/// Valid credential protection policies
#[derive(Debug, Serialize, Clone, Copy, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
#[repr(u8)]
pub enum CredentialProtectionPolicy {
    /// This reflects "FIDO_2_0" semantics. In this configuration, performing
    /// some form of user verification is optional with or without credentialID
    /// list. This is the default state of the credential if the extension is
    /// not specified.
    UserVerificationOptional = 0x1,
    /// In this configuration, credential is discovered only when its
    /// credentialID is provided by the platform or when some form of user
    /// verification is performed.
    UserVerificationOptionalWithCredentialIDList = 0x2,
    /// This reflects that discovery and usage of the credential MUST be
    /// preceded by some form of user verification.
    UserVerificationRequired = 0x3,
}

impl TryFrom<u8> for CredentialProtectionPolicy {
    type Error = &'static str;

    fn try_from(v: u8) -> Result<Self, Self::Error> {
        use CredentialProtectionPolicy::*;
        match v {
            0x1 => Ok(UserVerificationOptional),
            0x2 => Ok(UserVerificationOptionalWithCredentialIDList),
            0x3 => Ok(UserVerificationRequired),
            _ => Err("Invalid policy number"),
        }
    }
}

/// The desired options for the client's use of the `credProtect` extension
///
/// <https://fidoalliance.org/specs/fido-v2.1-rd-20210309/fido-client-to-authenticator-protocol-v2.1-rd-20210309.html#sctn-credProtect-extension>
#[derive(Debug, Serialize, Clone, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct CredProtect {
    /// The credential policy to enact
    pub credential_protection_policy: CredentialProtectionPolicy,
    /// Whether it is better for the authenticator to fail to create a
    /// credential rather than ignore the protection policy
    /// If no value is provided, the client treats it as `false`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub enforce_credential_protection_policy: Option<bool>,
}

/// Extension option inputs for PublicKeyCredentialCreationOptions.
///
/// Implements \[AuthenticatorExtensionsClientInputs\] from the spec.
#[derive(Debug, Serialize, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RequestRegistrationExtensions {
    /// The `credProtect` extension options
    #[serde(flatten, skip_serializing_if = "Option::is_none")]
    pub cred_protect: Option<CredProtect>,

    /// ⚠️  - Browsers do not support this!
    /// Uvm
    #[serde(skip_serializing_if = "Option::is_none")]
    pub uvm: Option<bool>,

    /// ⚠️  - This extension result is always unsigned, and only indicates if the
    /// browser *requests* a residentKey to be created. It has no bearing on the
    /// true rk state of the credential.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cred_props: Option<bool>,

    /// CTAP2.1 Minumum pin length
    #[serde(skip_serializing_if = "Option::is_none")]
    pub min_pin_length: Option<bool>,

    /// ⚠️  - Browsers support the *creation* of the secret, but not the retrieval of it.
    /// CTAP2.1 create hmac secret
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hmac_create_secret: Option<bool>,
}

impl Default for RequestRegistrationExtensions {
    fn default() -> Self {
        RequestRegistrationExtensions {
            cred_protect: None,
            uvm: Some(true),
            cred_props: Some(true),
            min_pin_length: None,
            hmac_create_secret: None,
        }
    }
}

// Unable to create from, because it's an out of crate struct
#[allow(clippy::from_over_into)]
#[cfg(feature = "wasm")]
impl Into<js_sys::Object> for &RequestRegistrationExtensions {
    fn into(self) -> js_sys::Object {
        use js_sys::Object;
        use wasm_bindgen::JsValue;

        let RequestRegistrationExtensions {
            cred_protect,
            uvm,
            cred_props,
            min_pin_length,
            hmac_create_secret,
        } = self;

        let obj = Object::new();

        if let Some(cred_protect) = cred_protect {
            let jsv = serde_wasm_bindgen::to_value(&cred_protect).unwrap();
            js_sys::Reflect::set(&obj, &"credProtect".into(), &jsv).unwrap();
        }

        if let Some(uvm) = uvm {
            js_sys::Reflect::set(&obj, &"uvm".into(), &JsValue::from_bool(*uvm)).unwrap();
        }

        if let Some(cred_props) = cred_props {
            js_sys::Reflect::set(&obj, &"credProps".into(), &JsValue::from_bool(*cred_props))
                .unwrap();
        }

        if let Some(min_pin_length) = min_pin_length {
            js_sys::Reflect::set(
                &obj,
                &"minPinLength".into(),
                &JsValue::from_bool(*min_pin_length),
            )
            .unwrap();
        }

        if let Some(hmac_create_secret) = hmac_create_secret {
            js_sys::Reflect::set(
                &obj,
                &"hmacCreateSecret".into(),
                &JsValue::from_bool(*hmac_create_secret),
            )
            .unwrap();
        }

        obj
    }
}

// ========== Auth exten ============

/// The inputs to the hmac secret if it was created during registration.
///
/// <https://fidoalliance.org/specs/fido-v2.1-ps-20210615/fido-client-to-authenticator-protocol-v2.1-ps-20210615.html#sctn-hmac-secret-extension>
#[derive(Debug, Serialize, Clone, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct HmacGetSecretInput {
    /// Retrieve a symmetric secrets from the authenticator with this input.
    pub output1: Base64UrlSafeData,
    /// Rotate the secret in the same operation.
    pub output2: Option<Base64UrlSafeData>,
}

/// Extension option inputs for PublicKeyCredentialRequestOptions
///
/// Implements \[AuthenticatorExtensionsClientInputs\] from the spec
#[derive(Debug, Serialize, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RequestAuthenticationExtensions {
    /// The `appid` extension options
    #[serde(skip_serializing_if = "Option::is_none")]
    pub appid: Option<String>,

    /// ⚠️  - Browsers do not support this!
    /// Uvm
    #[serde(skip_serializing_if = "Option::is_none")]
    pub uvm: Option<bool>,

    /// ⚠️  - Browsers do not support this!
    /// <https://bugs.chromium.org/p/chromium/issues/detail?id=1023225>
    /// Hmac get secret
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hmac_get_secret: Option<HmacGetSecretInput>,
}

// Unable to create from, because it's an out of crate struct
#[allow(clippy::from_over_into)]
#[cfg(feature = "wasm")]
impl Into<js_sys::Object> for &RequestAuthenticationExtensions {
    fn into(self) -> js_sys::Object {
        use js_sys::{Object, Uint8Array};
        use wasm_bindgen::JsValue;

        let RequestAuthenticationExtensions {
            // I don't think we care?
            appid: _,
            uvm,
            hmac_get_secret,
        } = self;

        let obj = Object::new();

        if let Some(uvm) = uvm {
            js_sys::Reflect::set(&obj, &"uvm".into(), &JsValue::from_bool(*uvm)).unwrap();
        }

        if let Some(HmacGetSecretInput { output1, output2 }) = hmac_get_secret {
            let hmac = Object::new();

            let o1 = Uint8Array::from(output1.as_slice());
            js_sys::Reflect::set(&hmac, &"output1".into(), &o1).unwrap();

            if let Some(output2) = output2 {
                let o2 = Uint8Array::from(output2.as_slice());
                js_sys::Reflect::set(&hmac, &"output2".into(), &o2).unwrap();
            }

            js_sys::Reflect::set(&obj, &"hmacGetSecret".into(), &hmac).unwrap();
        }

        obj
    }
}

/// The response to a hmac get secret request.
#[derive(Debug, Serialize, Clone, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub struct HmacGetSecretOutput {
    /// Output of HMAC(Salt 1 || Client Secret)
    pub output1: Base64UrlSafeData,
    /// Output of HMAC(Salt 2 || Client Secret)
    pub output2: Option<Base64UrlSafeData>,
}

/// <https://w3c.github.io/webauthn/#dictdef-authenticationextensionsclientoutputs>
/// The default option here for Options are None, so it can be derived
#[derive(Debug, Deserialize, Serialize, Clone, Default)]
pub struct AuthenticationExtensionsClientOutputs {
    /// Indicates whether the client used the provided appid extension
    #[serde(default)]
    pub appid: Option<bool>,
    /// The response to a hmac get secret request.
    #[serde(default)]
    pub hmac_get_secret: Option<HmacGetSecretOutput>,
}

#[cfg(feature = "wasm")]
impl From<web_sys::AuthenticationExtensionsClientOutputs>
    for AuthenticationExtensionsClientOutputs
{
    fn from(
        ext: web_sys::AuthenticationExtensionsClientOutputs,
    ) -> AuthenticationExtensionsClientOutputs {
        use js_sys::Uint8Array;

        let appid = js_sys::Reflect::get(&ext, &"appid".into())
            .ok()
            .and_then(|jv| jv.as_bool());

        let hmac_get_secret = js_sys::Reflect::get(&ext, &"hmacGetSecret".into())
            .ok()
            .and_then(|jv| {
                let output2 = js_sys::Reflect::get(&jv, &"output2".into())
                    .map(|v| Uint8Array::new(&v).to_vec())
                    .map(Base64UrlSafeData::from)
                    .ok();

                let output1 = js_sys::Reflect::get(&jv, &"output1".into())
                    .map(|v| Uint8Array::new(&v).to_vec())
                    .map(Base64UrlSafeData::from)
                    .ok();

                output1.map(|output1| HmacGetSecretOutput { output1, output2 })
            });

        AuthenticationExtensionsClientOutputs {
            appid,
            hmac_get_secret,
        }
    }
}

/// <https://www.w3.org/TR/webauthn-3/#sctn-authenticator-credential-properties-extension>
#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct CredProps {
    /// A user agent supplied hint that this credential *may* have created a resident key. It is
    /// retured from the user agent, not the authenticator meaning that this is an unreliable
    /// signal.
    ///
    /// Note that this extension is UNSIGNED and may have been altered by page javascript.
    pub rk: bool,
}

/// <https://w3c.github.io/webauthn/#dictdef-authenticationextensionsclientoutputs>
/// The default option here for Options are None, so it can be derived
#[derive(Debug, Deserialize, Serialize, Clone, Default)]
pub struct RegistrationExtensionsClientOutputs {
    /// Indicates whether the client used the provided appid extension
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub appid: Option<bool>,

    /// Indicates if the client believes it created a resident key. This
    /// property is managed by the webbrowser, and is NOT SIGNED and CAN NOT be trusted!
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub cred_props: Option<CredProps>,

    /// Indicates if the client successfully applied a HMAC Secret
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub hmac_secret: Option<bool>,

    /// Indicates if the client successfully applied a credential protection policy.
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub cred_protect: Option<CredentialProtectionPolicy>,

    /// Indicates the current minimum PIN length
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub min_pin_length: Option<u32>,
}

#[cfg(feature = "wasm")]
impl From<web_sys::AuthenticationExtensionsClientOutputs> for RegistrationExtensionsClientOutputs {
    fn from(
        ext: web_sys::AuthenticationExtensionsClientOutputs,
    ) -> RegistrationExtensionsClientOutputs {
        let appid = js_sys::Reflect::get(&ext, &"appid".into())
            .ok()
            .and_then(|jv| jv.as_bool());

        // Destructure "credProps":{"rk":false} from within a map.
        let cred_props = js_sys::Reflect::get(&ext, &"credProps".into())
            .ok()
            .and_then(|cred_props_struct| {
                js_sys::Reflect::get(&cred_props_struct, &"rk".into())
                    .ok()
                    .and_then(|jv| jv.as_bool())
                    .map(|rk| CredProps { rk })
            });

        let hmac_secret = js_sys::Reflect::get(&ext, &"hmac-secret".into())
            .ok()
            .and_then(|jv| jv.as_bool());

        let cred_protect = js_sys::Reflect::get(&ext, &"credProtect".into())
            .ok()
            .and_then(|jv| jv.as_f64())
            .and_then(|f| CredentialProtectionPolicy::try_from(f as u8).ok());

        let min_pin_length = js_sys::Reflect::get(&ext, &"minPinLength".into())
            .ok()
            .and_then(|jv| jv.as_f64())
            .map(|f| f as u32);

        RegistrationExtensionsClientOutputs {
            appid,
            cred_props,
            hmac_secret,
            cred_protect,
            min_pin_length,
        }
    }
}

/// The result state of an extension as returned from the authenticator.
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub enum ExtnState<T>
where
    T: Clone + std::fmt::Debug,
{
    /// This extension was not requested, and so no result was provided.
    #[default]
    NotRequested,
    /// The extension was requested, and the authenticator did NOT act on it.
    Ignored,
    /// The extension was requested, and the authenticator correctly responded.
    Set(T),
    /// The extension was not requested, and the authenticator sent an unsolicited extension value.
    Unsolicited(T),
    /// ⚠️  WARNING: The data in this extension is not signed cryptographically, and can not be
    /// trusted for security assertions. It MAY be used for UI/UX hints.
    Unsigned(T),
}

/// The set of extensions that were registered by this credential.
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct RegisteredExtensions {
    // ⚠️  It's critical we place serde default here so that we
    // can deserialise in the future as we add new types!
    /// The state of the cred_protect extension
    #[serde(default)]
    pub cred_protect: ExtnState<CredentialProtectionPolicy>,
    /// The state of the hmac-secret extension, if it was created
    #[serde(default)]
    pub hmac_create_secret: ExtnState<bool>,
    /// The state of the client appid extensions
    #[serde(default)]
    pub appid: ExtnState<bool>,
    /// The state of the client credential properties extension
    #[serde(default)]
    pub cred_props: ExtnState<CredProps>,
}

impl RegisteredExtensions {
    /// Yield an empty set of registered extensions
    pub fn none() -> Self {
        RegisteredExtensions {
            cred_protect: ExtnState::NotRequested,
            hmac_create_secret: ExtnState::NotRequested,
            appid: ExtnState::NotRequested,
            cred_props: ExtnState::NotRequested,
        }
    }
}

/// The set of extensions that were provided by the client during authentication
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct AuthenticationExtensions {}
