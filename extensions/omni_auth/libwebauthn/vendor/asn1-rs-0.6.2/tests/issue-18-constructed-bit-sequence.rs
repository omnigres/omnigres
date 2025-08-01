#![cfg(feature = "std")]

use std::borrow::Cow;

use asn1_rs::*;

#[derive(DerSequence, Debug, PartialEq)]
struct Person {
    name: String,
    age: u16,
}

#[test]
fn issue_18_1() {
    // create a sequence from random data
    let seq = Sequence::new(Cow::Borrowed(&[2, 2, 18, 52, 2, 2, 86, 12]));

    // now serialize a [2] IMPLICIT Person
    type T2<'a> = TaggedValue<Sequence<'a>, Error, Implicit, { Class::UNIVERSAL }, 2>;
    let tagged = T2::implicit(seq);

    let result = tagged.to_der_vec().expect("Could not serialize sequence");

    let (_, header) = Header::from_der(&result).expect("could not parse serialized data");
    assert!(header.is_constructed());
}
