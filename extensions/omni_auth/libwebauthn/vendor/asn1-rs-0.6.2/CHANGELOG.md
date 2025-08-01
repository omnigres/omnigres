# Change Log

## [Unreleased][unreleased]

### Changed/Fixed

### Added

### Thanks

## 0.6.2

### Changed/Fixed

Important:

- Fix a potential panic when using derived parsers, when using custom errors (see #40)
  This affects only auto-derived parsers specifying a custom error, and parsing incomplete data.

Fixed:

- Fix wrong encoding of large tags (#43)
- Fix wrong encoding of optional TaggedImplicit object (#42)

General:
- Add licences to sub-crates (#38)
- Refactor CI (#36, #41)
- Updates license field to valid SPDX format (#34)

### Thanks

- Daniel McCarney, Łukasz Wojniłowicz, Isaiah Becker-Mayer, Philip Ye

## 0.6.1

### Changed/Fixed

- Provide implementations for `Option<Any>::from_der`

## 0.6.0

### Changed/Fixed

General:
- Set MSRV to 1.67.0
- Add PartialEq to SequenceOf and SetOf
- Implement traits for SequenceOf and SetOf to improve usability
- Fix receiver lifetimes in `Any` methods
- Implement `BmpString::try_from` for &Any (so it does not need to consume input) (#26)
- oid: change macro to expect dot-separated literals (#28)
- Fix wrong tag in encoding of SET OF (#30)
- Option: require T::Tagged, and check tag before constraints (#27)
- Add missing constructed bit when serializing [2] IMPLICIT) (#18)
- Add methods to convert Any to Real
- Add tag for CHARACTER STRING (29)
- Fix method `Any::as_generalstring` (wrong return type)
- Add method `Any::as_bmpstring`
- Fix clippy warnings (1.76.0)

Dependencies updates:
- pem to 3.0
- hex-literal to 0.4
- syn to 2.0
- examples: drop circular dev-dependency caused by oid-registry

### Thanks

- Sergio Benitez, Andrey Chesnokov

## 0.5.2

### Changed/Fixed

- Fix decoding of integers: check if value will wrap if integer is signed
- Fix encoding of integers (add 0x00 prefix when required, and remove extra 0xff for negative integers)
- Fix a small math error in GeneralizedTime
- Introduce trait GetObjectContent, use `from_ber` when skipping BER content (closes #14)

### Thanks

- Nadja Reitzenstein, Christian Speich

## 0.5.1

Minor fixes:

- Fix constraints too strict on `TaggedValue::FromDer`, do not auto-derive
- Update oid-registry
- Fix `Any::as_relative_oid` to take a reference (and not consume input)

derive:

- Add special case handler for alias to Any
- Add support for DEFAULT attribute

## 0.5.0

This release adds some new methods and custom derive attributes.
It also adds a lot of tests to improve code coverage.

asn1-rs:

- Add helper types for Application/Private tagged values
- Any: add methods `from_ber_and_then` (and `_der`)
- TaggedParser: add documentation for `from_ber_and_then` (and `_der`)
- Oid: add method `starts_with`
- Fix documentation of application and private tagged helpers
- Fix clippy warnings

derive:

- Add custom derive BerAlias and DerAlias

coverage:

- Add many tests to improve coverage

## 0.4.2

Bugfix release:
- Remove explicit output lifetime in traits
- Fix wrong encoding `BmpString` when using `ToDer`
- Fix parsing of some EmbeddedPdv subtypes
- Fix encoded length for Enumerated
- Add missing `DerAutoDerive` impl for bool
- Add missing `DerAutoDerive` impl for f32/f64
- Remove redundant check, `Any::from_der` checks than length is definite
- Length: fix potential bug when adding Length + Indefinite
- Fix inverted logic in `Header::assert_definite()`

## 0.4.1

Minor fix:
- add missing file in distribution (fix docs.rs build)

## 0.4.0

asn1-rs:

- Add generic error parameter in traits and in types
  - This was added for all types except a few (like `Vec<T>` or `BTreeSet<T>`) due to
    Rust compiler limitations
- Add `DerAutoDerive` trait to control manual/automatic implementation of `FromDer`
  - This allow controlling automatic trait implementation, and providing manual
    implementations of both `FromDer` and `CheckDerConstraints`
- UtcTime: Introduce utc_adjusted_date() to map 2 chars years date to 20/21 centuries date (#9)

derive:

- Add attributes to simplify deriving EXPLICIT, IMPLICIT and OPTIONAL
- Add support for different tag classes (like APPLICATION or PRIVATE)
- Add support for custom errors and mapping errors
- Add support for deriving BER/DER SET
- DerDerive: derive both CheckDerConstraints and FromDer

documentation:

- Add doc modules for recipes and for custom derive attributes
- Add note on trailing bytes being ignored in sequence
- Improve documentation for notation with braces in TaggedValue
- Improve documentation
