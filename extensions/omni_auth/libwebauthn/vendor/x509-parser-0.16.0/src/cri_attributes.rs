use crate::{
    error::{X509Error, X509Result},
    extensions::X509Extension,
};

use asn1_rs::{Error, FromDer, Header, Oid, Sequence, Tag};
use nom::combinator::{all_consuming, complete};
use nom::multi::many0;
use nom::Err;
use oid_registry::*;
use std::collections::HashMap;

/// Attributes for Certification Request
#[derive(Clone, Debug, PartialEq)]
pub struct X509CriAttribute<'a> {
    pub oid: Oid<'a>,
    pub value: &'a [u8],
    pub(crate) parsed_attribute: ParsedCriAttribute<'a>,
}

impl<'a> FromDer<'a, X509Error> for X509CriAttribute<'a> {
    fn from_der(i: &'a [u8]) -> X509Result<X509CriAttribute> {
        Sequence::from_ber_and_then(i, |i| {
            let (i, oid) = Oid::from_der(i)?;
            let value_start = i;
            let (i, hdr) = Header::from_der(i)?;
            if hdr.tag() != Tag::Set {
                return Err(Err::Error(Error::BerTypeError));
            };

            let (i, parsed_attribute) = crate::cri_attributes::parser::parse_attribute(i, &oid)
                .map_err(|_| Err::Error(Error::BerValueError))?;
            let attribute = X509CriAttribute {
                oid,
                value: &value_start[..value_start.len() - i.len()],
                parsed_attribute,
            };
            Ok((i, attribute))
        })
        .map_err(|_| X509Error::InvalidAttributes.into())
    }
}

impl<'a> X509CriAttribute<'a> {
    /// Return the attribute type or `UnsupportedAttribute` if the attribute is unknown.
    #[inline]
    pub fn parsed_attribute(&self) -> &ParsedCriAttribute<'a> {
        &self.parsed_attribute
    }
}

/// Section 3.1 of rfc 5272
#[derive(Clone, Debug, PartialEq)]
pub struct ExtensionRequest<'a> {
    pub extensions: Vec<X509Extension<'a>>,
}

impl<'a> FromDer<'a, X509Error> for ExtensionRequest<'a> {
    fn from_der(i: &'a [u8]) -> X509Result<'a, Self> {
        parser::parse_extension_request(i).map_err(Err::convert)
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ChallengePassword(pub String);

/// Attributes for Certification Request
#[derive(Clone, Debug, PartialEq)]
pub enum ParsedCriAttribute<'a> {
    ChallengePassword(ChallengePassword),
    ExtensionRequest(ExtensionRequest<'a>),
    UnsupportedAttribute,
}

pub(crate) mod parser {
    use crate::cri_attributes::*;
    use der_parser::der::{
        parse_der_bmpstring, parse_der_printablestring, parse_der_t61string,
        parse_der_universalstring, parse_der_utf8string,
    };
    use lazy_static::lazy_static;
    use nom::branch::alt;
    use nom::combinator::map;

    type AttrParser = fn(&[u8]) -> X509Result<ParsedCriAttribute>;

    lazy_static! {
        static ref ATTRIBUTE_PARSERS: HashMap<Oid<'static>, AttrParser> = {
            macro_rules! add {
                ($m:ident, $oid:ident, $p:ident) => {
                    $m.insert($oid, $p as AttrParser);
                };
            }

            let mut m = HashMap::new();
            add!(m, OID_PKCS9_EXTENSION_REQUEST, parse_extension_request_attr);
            add!(
                m,
                OID_PKCS9_CHALLENGE_PASSWORD,
                parse_challenge_password_attr
            );
            m
        };
    }

    // look into the parser map if the extension is known, and parse it
    // otherwise, leave it as UnsupportedExtension
    pub(crate) fn parse_attribute<'a>(
        i: &'a [u8],
        oid: &Oid,
    ) -> X509Result<'a, ParsedCriAttribute<'a>> {
        if let Some(parser) = ATTRIBUTE_PARSERS.get(oid) {
            parser(i)
        } else {
            Ok((i, ParsedCriAttribute::UnsupportedAttribute))
        }
    }

    pub(super) fn parse_extension_request(i: &[u8]) -> X509Result<ExtensionRequest> {
        crate::extensions::parse_extension_sequence(i)
            .map(|(i, extensions)| (i, ExtensionRequest { extensions }))
    }

    fn parse_extension_request_attr(i: &[u8]) -> X509Result<ParsedCriAttribute> {
        map(
            parse_extension_request,
            ParsedCriAttribute::ExtensionRequest,
        )(i)
    }

    // RFC 2985, 5.4.1 Challenge password
    //    challengePassword ATTRIBUTE ::= {
    //            WITH SYNTAX DirectoryString {pkcs-9-ub-challengePassword}
    //            EQUALITY MATCHING RULE caseExactMatch
    //            SINGLE VALUE TRUE
    //            ID pkcs-9-at-challengePassword
    //    }
    // RFC 5280, 4.1.2.4.  Issuer
    //    DirectoryString ::= CHOICE {
    //          teletexString           TeletexString (SIZE (1..MAX)),
    //          printableString         PrintableString (SIZE (1..MAX)),
    //          universalString         UniversalString (SIZE (1..MAX)),
    //          utf8String              UTF8String (SIZE (1..MAX)),
    //          bmpString               BMPString (SIZE (1..MAX))
    //    }
    pub(super) fn parse_challenge_password(i: &[u8]) -> X509Result<ChallengePassword> {
        let (rem, obj) = match alt((
            parse_der_utf8string,
            parse_der_printablestring,
            parse_der_universalstring,
            parse_der_bmpstring,
            parse_der_t61string, // == teletexString
        ))(i)
        {
            Ok((rem, obj)) => (rem, obj),
            Err(_) => return Err(Err::Error(X509Error::InvalidAttributes)),
        };
        match obj.content.as_str() {
            Ok(s) => Ok((rem, ChallengePassword(s.to_string()))),
            Err(_) => Err(Err::Error(X509Error::InvalidAttributes)),
        }
    }

    fn parse_challenge_password_attr(i: &[u8]) -> X509Result<ParsedCriAttribute> {
        map(
            parse_challenge_password,
            ParsedCriAttribute::ChallengePassword,
        )(i)
    }
}

pub(crate) fn parse_cri_attributes(i: &[u8]) -> X509Result<Vec<X509CriAttribute>> {
    let (i, hdr) = Header::from_der(i).map_err(|_| Err::Error(X509Error::InvalidAttributes))?;
    if hdr.is_contextspecific() && hdr.tag().0 == 0 {
        all_consuming(many0(complete(X509CriAttribute::from_der)))(i)
    } else {
        Err(Err::Error(X509Error::InvalidAttributes))
    }
}
