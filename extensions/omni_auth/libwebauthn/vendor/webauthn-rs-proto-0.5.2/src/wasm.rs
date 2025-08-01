//! Extensions for wasm types that are not part of web-sys
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
extern "C" {
    /// Public Key Credential Extension
    pub type PublicKeyCredentialExt;

    #[wasm_bindgen(static_method_of = PublicKeyCredentialExt, js_class = "PublicKeyCredential", js_name = isConditionalMediationAvailable, catch)]
    /// Is Conditional Mediation Available
    pub fn is_conditional_mediation_available() -> Result<::js_sys::Promise, JsValue>;

    #[wasm_bindgen(static_method_of = PublicKeyCredentialExt, js_class = "PublicKeyCredential", js_name = isExternalCTAP2SecurityKeySupported, catch)]
    /// Is External Ctap2 SecurityKey Supported
    pub fn is_external_ctap2_securitykey_supported() -> Result<::js_sys::Promise, JsValue>;
}
