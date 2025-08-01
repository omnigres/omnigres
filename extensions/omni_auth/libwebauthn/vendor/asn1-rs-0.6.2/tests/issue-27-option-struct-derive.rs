use asn1_rs::*;

#[derive(DerSequence, Debug, PartialEq)]
struct TestBool {
    a: u16,
    b: Option<bool>,
    c: u32,
}

#[test]
fn issue_27_1() {
    let x = TestBool {
        a: 0x1234,
        b: None,
        c: 0x5678,
    };

    let expected = &[48, 8, 2, 2, 18, 52, 2, 2, 86, 120];

    let (_, val) = TestBool::from_der(expected).unwrap();
    assert_eq!(val, x);
}

#[test]
fn issue_27_2() {
    let x = TestBool {
        a: 0x1234,
        b: Some(true),
        c: 0x5678,
    };

    let expected = &[48, 11, 2, 2, 18, 52, 1, 1, 255, 2, 2, 86, 120];

    let (_, val) = TestBool::from_der(expected).unwrap();
    assert_eq!(val, x);
}

#[derive(DerSequence, Debug, PartialEq)]
struct TestInt {
    a: u16,
    b: Option<u32>,
    c: bool,
}

#[test]
fn issue_27_3() {
    let x = TestInt {
        a: 0x1234,
        b: None,
        c: true,
    };

    let expected = &[48, 7, 2, 2, 18, 52, 1, 1, 255];

    let (_, val) = TestInt::from_der(expected).unwrap();
    assert_eq!(val, x);
}

#[test]
fn issue_27_4() {
    let x = TestInt {
        a: 0x1234,
        b: Some(0x5678),
        c: true,
    };

    let expected = &[48, 11, 2, 2, 18, 52, 2, 2, 86, 120, 1, 1, 255];

    let (_, val) = TestInt::from_der(expected).unwrap();
    assert_eq!(val, x);
}
