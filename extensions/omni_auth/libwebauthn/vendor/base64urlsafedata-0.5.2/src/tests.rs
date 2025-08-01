use super::*;

macro_rules! from_json_test {
    ($($name:ident: $value:expr,)*) => {
        $(
            #[test]
            fn $name() {
                let (input, expected): (&str, &[u8]) = $value;
                assert_eq!(serde_json::from_str::<Base64UrlSafeData>(input).unwrap(), expected);
                assert_eq!(serde_json::from_str::<HumanBinaryData>(input).unwrap(), expected);
            }
        )*
    };
}

macro_rules! from_invalid_json_test {
    ($($name:ident: $value:expr,)*) => {
        $(
            #[test]
            fn $name() {
                let input: &str = $value;
                assert!(serde_json::from_str::<Base64UrlSafeData>(input).is_err());
                assert!(serde_json::from_str::<HumanBinaryData>(input).is_err());
            }
        )*
    };
}

macro_rules! from_cbor_test {
    ($($name:ident: $value:expr,)*) => {
        $(
            #[test]
            fn $name() {
                let (input, expected): (&[u8], &[u8]) = $value;
                assert_eq!(serde_cbor_2::from_slice::<Base64UrlSafeData>(input).unwrap(), expected);
                assert_eq!(serde_cbor_2::from_slice::<HumanBinaryData>(input).unwrap(), expected);
            }
        )*
    };
}

macro_rules! from_invalid_cbor_test {
    ($($name:ident: $value:expr,)*) => {
        $(
            #[test]
            fn $name() {
                let input: &[u8] = $value;
                assert!(serde_cbor_2::from_slice::<Base64UrlSafeData>(input).is_err());
                assert!(serde_cbor_2::from_slice::<HumanBinaryData>(input).is_err());
            }
        )*
    };
}

from_json_test! {
    from_json_empty_array: ("[]", &[]),
    from_json_as_array_number: ("[0,1,2,255]", &[0x00, 0x01, 0x02, 0xFF]),
    from_json_as_array_number_whitespace: ("[0, 1, 2, 255]", &[0x00, 0x01, 0x02, 0xFF]),
    from_json_empty_string: ("\"\"", &[]),
    from_json_b64_urlsafe_nonpadded: ("\"AAEC_w\"", &[0x00, 0x01, 0x02, 0xFF]),
    from_json_b64_urlsafe_padded: ("\"AAEC_w==\"", &[0x00, 0x01, 0x02, 0xFF]),
    from_json_b64_standard_nonpadded: ("\"AAEC/w\"", &[0x00, 0x01, 0x02, 0xFF]),
    from_json_b64_standard_padded: ("\"AAEC/w==\"", &[0x00, 0x01, 0x02, 0xFF]),
}

from_invalid_json_test! {
    from_json_empty: "",
    from_json_null: "null",
    from_json_number: "1",
    from_json_empty_map: "{}",
    from_json_map: "{\"1\": \"AAEC_w\"}",
}

from_cbor_test! {
    from_cbor_bytes: (&[
        0x44, // bytes(4)
        0x00, 0x01, 0x02, 0xFF,
    ], &[0x00, 0x01, 0x02, 0xFF]),
    from_cbor_array: (&[
        0x84, // array(4)
        0x00, // 0
        0x01, // 1
        0x02, // 2
        0x18, 0xff, // 0xff
    ], &[0x00, 0x01, 0x02, 0xFF]),
    from_cbor_empty_array: (&[0x80], &[]), // array(0)
    from_cbor_empty_string: (&[0x60], &[]), // text(0)
    from_cbor_string_b64_urlsafe_nonpadded: (&[
        0x66, // text(6)
        0x41, 0x41, 0x45, 0x43, 0x5F, 0x77, // "AAEC_w"
    ], &[0x00, 0x01, 0x02, 0xFF]),
    from_cbor_string_b64_urlsafe_padded: (&[
        0x68, // text(8)
        0x41, 0x41, 0x45, 0x43, 0x5F, 0x77, 0x3D, 0x3D // "AAEC_w=="
    ], &[0x00, 0x01, 0x02, 0xFF]),
    from_cbor_string_b64_standard_nonpadded: (&[
        0x66, // text(6)
        0x41, 0x41, 0x45, 0x43, 0x2F, 0x77, // "AAEC/w"
    ], &[0x00, 0x01, 0x02, 0xFF]),
    from_cbor_string_b64_standard_padded: (&[
        0x68, // text(8)
        0x41, 0x41, 0x45, 0x43, 0x2F, 0x77, 0x3D, 0x3D // "AAEC/w=="
    ], &[0x00, 0x01, 0x02, 0xFF]),
}

from_invalid_cbor_test! {
    from_seq_string: &[0x82, 0x61, 0x61, 0x61, 0x62],
    from_empty: &[],
    from_positive_int: &[0x01],
    from_negative_int: &[0x20],
    from_seq_negative_int: &[0x82, 0x20, 0x21],
    from_seq_positive_and_negative_int: &[0x82, 0x01, 0x20],
}

#[test]
fn to_json() {
    let input = [0x00, 0x01, 0x02, 0xff];

    // JSON output should always be a base64 string
    assert_eq!(
        serde_json::to_string(&Base64UrlSafeData::from(input)).unwrap(),
        "\"AAEC_w\"",
    );
    assert_eq!(
        serde_json::to_string(&HumanBinaryData::from(input)).unwrap(),
        "\"AAEC_w\"",
    );
}

#[test]
fn to_cbor() {
    let input = [0x00, 0x01, 0x02, 0xff];

    // Base64UrlSafeData CBOR output should be a base64 encoded string
    assert_eq!(
        serde_cbor_2::to_vec(&Base64UrlSafeData::from(input)).unwrap(),
        vec![
            0x66, // text(6)
            0x41, 0x41, 0x45, 0x43, 0x5F, 0x77 // "AAEC_w"
        ]
    );

    // HumanBinaryData CBOR output should be a bytes
    assert_eq!(
        serde_cbor_2::to_vec(&HumanBinaryData::from(input)).unwrap(),
        vec![
            0x44, // bytes(4)
            0x00, 0x01, 0x02, 0xff
        ]
    );
}

#[test]
fn interop_from() {
    let input = [0x00, 0x01, 0x02, 0xff];
    let a = Base64UrlSafeData::from(input.as_ref());
    let b = HumanBinaryData::from(input.as_ref());

    let c = Base64UrlSafeData::from(b.clone());
    assert_eq!(a, c);
    let d = HumanBinaryData::from(a);
    assert_eq!(b, d);
}

#[test]
fn interop_equality() {
    let input = [0x00, 0x01, 0x02, 0xff];
    let other = [0xff, 0x00, 0x01, 0x02];

    assert_eq!(
        Base64UrlSafeData::from(input.as_ref()),
        HumanBinaryData::from(input.as_ref()),
    );

    assert_eq!(
        HumanBinaryData::from(input.as_ref()),
        Base64UrlSafeData::from(input.as_ref()),
    );

    assert_eq!(input, Base64UrlSafeData::from(input.as_ref()));
    assert_eq!(Base64UrlSafeData::from(input.as_ref()), input);
    assert_eq!(input, HumanBinaryData::from(input.as_ref()));
    assert_eq!(HumanBinaryData::from(input.as_ref()), input);

    assert_ne!(
        Base64UrlSafeData::from(input.as_ref()),
        HumanBinaryData::from(other.as_ref()),
    );

    assert_ne!(
        HumanBinaryData::from(input.as_ref()),
        Base64UrlSafeData::from(other.as_ref()),
    );

    assert_ne!(input, Base64UrlSafeData::from(other.as_ref()));
    assert_ne!(Base64UrlSafeData::from(other.as_ref()), input);
    assert_ne!(input, HumanBinaryData::from(other.as_ref()));
    assert_ne!(HumanBinaryData::from(other.as_ref()), input);
}

#[test]
fn interop_vec() {
    let mut a = Base64UrlSafeData::from([0, 1, 2, 3]);
    a.push(4);
    assert_eq!(vec![0, 1, 2, 3, 4], a);
    assert_eq!(5, a.len());

    let mut a = HumanBinaryData::from([0, 1, 2, 3]);
    a.push(4);
    assert_eq!(vec![0, 1, 2, 3, 4], a);
    assert_eq!(5, a.len());
}
