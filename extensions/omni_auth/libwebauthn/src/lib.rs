use std::ffi::{CStr, CString, c_char};
use std::str::FromStr;
use webauthn_rs::prelude::*;
use uuid::Uuid;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn webauthn(
    rp_id: *const c_char,
    rp_origin: *const c_char,
) -> *const Webauthn {
    let rp_origin_url =
        Url::parse(CStr::from_ptr(rp_origin).to_str().unwrap()).expect("Invalid URL");
    let builder = WebauthnBuilder::new(CStr::from_ptr(rp_id).to_str().unwrap(), &rp_origin_url)
        .expect("Invalid configuration");
    let webauthn = builder.build().expect("Failed to build");
    Box::leak(Box::new(webauthn))
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn webauthn_free(webauthn: *mut Webauthn) {
    drop(Box::from_raw(webauthn));
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn webauthn_start_passkey_registration(
    webauthn_ptr: *mut Webauthn,
    user_unique_id: *const c_char,
    user_name: *const c_char,
    user_display_name: *const c_char,
) -> *const c_char {
    let webauthn = Box::from_raw(webauthn_ptr);
    let uuid = Uuid::from_str(CStr::from_ptr(user_unique_id).to_str().unwrap()).unwrap();
    let result = webauthn.start_passkey_registration(
        uuid,
        CStr::from_ptr(user_name).to_str().unwrap(),
        CStr::from_ptr(user_display_name).to_str().unwrap(),
        None,
    );
    Box::leak(webauthn);
    let (ccr, skr) = result.expect("Failed to start registration.");
    CString::new(serde_json::to_string(&ccr).unwrap())
        .unwrap()
        .into_raw()
}
