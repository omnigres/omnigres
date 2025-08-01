//! Types related to attestation (Registration)

#[cfg(feature = "wasm")]
use base64::Engine;
use base64urlsafedata::Base64UrlSafeData;
use serde::{Deserialize, Serialize};

use crate::extensions::{RegistrationExtensionsClientOutputs, RequestRegistrationExtensions};
use crate::options::*;
#[cfg(feature = "wasm")]
use crate::BASE64_ENGINE;

/// <https://w3c.github.io/webauthn/#dictionary-makecredentialoptions>
#[derive(Debug, Serialize, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PublicKeyCredentialCreationOptions {
    /// The relying party
    pub rp: RelyingParty,
    /// The user.
    pub user: User,
    /// The one-time challenge for the credential to sign.
    pub challenge: Base64UrlSafeData,
    /// The set of cryptographic types allowed by this server.
    pub pub_key_cred_params: Vec<PubKeyCredParams>,

    /// The timeout for the authenticator to stop accepting the operation
    #[serde(skip_serializing_if = "Option::is_none")]
    pub timeout: Option<u32>,

    /// Credential ID's that are excluded from being able to be registered.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub exclude_credentials: Option<Vec<PublicKeyCredentialDescriptor>>,

    /// Criteria defining which authenticators may be used in this operation.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub authenticator_selection: Option<AuthenticatorSelectionCriteria>,

    /// Hints defining which credentials may be used in this operation.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hints: Option<Vec<PublicKeyCredentialHints>>,

    /// The requested attestation level from the device.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub attestation: Option<AttestationConveyancePreference>,

    /// The list of attestation formats that the RP will accept.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub attestation_formats: Option<Vec<AttestationFormat>>,

    /// Non-standard extensions that may be used by the browser/authenticator.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub extensions: Option<RequestRegistrationExtensions>,
}

/// A JSON serializable challenge which is issued to the user's web browser
/// for handling. This is meant to be opaque, that is, you should not need
/// to inspect or alter the content of the struct - you should serialise it
/// and transmit it to the client only.
#[derive(Debug, Serialize, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CreationChallengeResponse {
    /// The options.
    pub public_key: PublicKeyCredentialCreationOptions,
}

#[cfg(feature = "wasm")]
impl From<CreationChallengeResponse> for web_sys::CredentialCreationOptions {
    fn from(ccr: CreationChallengeResponse) -> Self {
        use js_sys::{Array, Object, Uint8Array};
        use wasm_bindgen::JsValue;

        let chal = Uint8Array::from(ccr.public_key.challenge.as_slice());
        let userid = Uint8Array::from(ccr.public_key.user.id.as_slice());

        let jsv = serde_wasm_bindgen::to_value(&ccr).unwrap();

        let pkcco = js_sys::Reflect::get(&jsv, &"publicKey".into()).unwrap();
        js_sys::Reflect::set(&pkcco, &"challenge".into(), &chal).unwrap();

        let user = js_sys::Reflect::get(&pkcco, &"user".into()).unwrap();
        js_sys::Reflect::set(&user, &"id".into(), &userid).unwrap();

        if let Some(extensions) = ccr.public_key.extensions {
            let obj: Object = (&extensions).into();
            js_sys::Reflect::set(&pkcco, &"extensions".into(), &obj).unwrap();
        }

        if let Some(exclude_credentials) = ccr.public_key.exclude_credentials {
            // There must be an array of these in the jsv ...
            let exclude_creds: Array = exclude_credentials
                .iter()
                .map(|ac| {
                    let obj = Object::new();
                    js_sys::Reflect::set(
                        &obj,
                        &"type".into(),
                        &JsValue::from_str(ac.type_.as_str()),
                    )
                    .unwrap();

                    js_sys::Reflect::set(&obj, &"id".into(), &Uint8Array::from(ac.id.as_slice()))
                        .unwrap();

                    if let Some(transports) = &ac.transports {
                        let tarray: Array = transports
                            .iter()
                            .map(|trs| serde_wasm_bindgen::to_value(trs).unwrap())
                            .collect();

                        js_sys::Reflect::set(&obj, &"transports".into(), &tarray).unwrap();
                    }

                    obj
                })
                .collect();

            js_sys::Reflect::set(&pkcco, &"excludeCredentials".into(), &exclude_creds).unwrap();
        }

        web_sys::CredentialCreationOptions::from(jsv)
    }
}

/// <https://w3c.github.io/webauthn/#authenticatorattestationresponse>
#[derive(Debug, Serialize, Clone, Deserialize)]
pub struct AuthenticatorAttestationResponseRaw {
    /// <https://w3c.github.io/webauthn/#dom-authenticatorattestationresponse-attestationobject>
    #[serde(rename = "attestationObject")]
    pub attestation_object: Base64UrlSafeData,

    /// <https://w3c.github.io/webauthn/#dom-authenticatorresponse-clientdatajson>
    #[serde(rename = "clientDataJSON")]
    pub client_data_json: Base64UrlSafeData,

    /// <https://w3c.github.io/webauthn/#dom-authenticatorattestationresponse-gettransports>
    #[serde(default)]
    pub transports: Option<Vec<AuthenticatorTransport>>,
}

/// A client response to a registration challenge. This contains all required
/// information to assess and assert trust in a credential's legitimacy, followed
/// by registration to a user.
///
/// You should not need to handle the inner content of this structure - you should
/// provide this to the correctly handling function of Webauthn only.
/// <https://w3c.github.io/webauthn/#iface-pkcredential>
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct RegisterPublicKeyCredential {
    /// The id of the PublicKey credential, likely in base64.
    ///
    /// This is NEVER actually
    /// used in a real registration, because the true credential ID is taken from the
    /// attestation data.
    pub id: String,
    /// The id of the credential, as binary.
    ///
    /// This is NEVER actually
    /// used in a real registration, because the true credential ID is taken from the
    /// attestation data.
    #[serde(rename = "rawId")]
    pub raw_id: Base64UrlSafeData,
    /// <https://w3c.github.io/webauthn/#dom-publickeycredential-response>
    pub response: AuthenticatorAttestationResponseRaw,
    /// The type of credential.
    #[serde(rename = "type")]
    pub type_: String,
    /// Unsigned Client processed extensions.
    #[serde(default)]
    pub extensions: RegistrationExtensionsClientOutputs,
}

#[cfg(feature = "wasm")]
impl From<web_sys::PublicKeyCredential> for RegisterPublicKeyCredential {
    fn from(data: web_sys::PublicKeyCredential) -> RegisterPublicKeyCredential {
        use js_sys::Uint8Array;

        // AuthenticatorAttestationResponse has getTransports but web_sys isn't exposing it?
        let transports = None;

        // First, we have to b64 some data here.
        // data.raw_id
        let data_raw_id =
            Uint8Array::new(&js_sys::Reflect::get(&data, &"rawId".into()).unwrap()).to_vec();

        let data_response = js_sys::Reflect::get(&data, &"response".into()).unwrap();
        let data_response_attestation_object = Uint8Array::new(
            &js_sys::Reflect::get(&data_response, &"attestationObject".into()).unwrap(),
        )
        .to_vec();

        let data_response_client_data_json = Uint8Array::new(
            &js_sys::Reflect::get(&data_response, &"clientDataJSON".into()).unwrap(),
        )
        .to_vec();

        let data_extensions = data.get_client_extension_results();

        RegisterPublicKeyCredential {
            id: BASE64_ENGINE.encode(&data_raw_id),
            raw_id: data_raw_id.into(),
            response: AuthenticatorAttestationResponseRaw {
                attestation_object: data_response_attestation_object.into(),
                client_data_json: data_response_client_data_json.into(),
                transports,
            },
            type_: "public-key".to_string(),
            extensions: data_extensions.into(),
        }
    }
}
