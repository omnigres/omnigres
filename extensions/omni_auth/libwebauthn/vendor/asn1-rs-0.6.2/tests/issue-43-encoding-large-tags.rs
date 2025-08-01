use asn1_rs::{Any, FromDer, Integer, Tag, ToDer};

#[test]
fn encode_large_tag() {
    const EXPECTED_TAG: u32 = 0x41424344;
    let data = Integer::from(1).to_der_vec().unwrap();
    let any = &Any::from_tag_and_data(Tag::from(EXPECTED_TAG), &data);
    let tmp = any.to_der_vec().unwrap();

    let expect = Tag::from(EXPECTED_TAG);
    let actual = Any::from_der(&tmp).unwrap().1.tag();

    assert_eq!(expect, actual, "expected tag {expect}, found tag {actual}");
}
