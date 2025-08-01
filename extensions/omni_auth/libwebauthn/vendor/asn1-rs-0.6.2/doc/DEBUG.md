# Debugging

To debug parsing errors, the `debug` feature can be used to print any error on stderr:
```shell
$ ./fail_debug
asn1_rs::asn1_types::sequence::Sequence ↯ Parsing failed at location:
00000000	30 81 04 00 00                                  	0�...
asn1_rs::asn1_types::sequence::Sequence ↯ T::from_der failed: Parsing requires 2 bytes/chars
```
In the above example, the parser tries to read a `Sequence` but input is incomplete (missing at least 2 bytes).

The `debug` feature will print errors. To add the full trace of all parsing functions, use the `trace` feature:
```shell
$ ./fail_trace
u32 ⤷ T::from_der
u32 ⤷ input (len=3, type=asn1_rs::asn1_types::any::Any)
u32 ⤶ Parsed 3 bytes, 0 remaining
u32 ⤷ Conversion to uint
u32 ↯ T::from_der failed: Parsing Error: UnexpectedTag { expected: Some(Tag(2)), actual: Tag(1) }
bool ↯ T::from_der failed: Parsing Error: DerConstraintFailed(InvalidBoolean)
```
In this example, the parser tries to read an `u32`. It is first read as `Any` with sucess, however the conversion to `u32` fails because of a wrong tag. See below for details on how to interpret output.

Note that the `trace` feature is very verbose, and can generate a huge amount of logs on large inputs.

## Interpretating output

When interpreting the trace output, knowing how `asn1-rs` works is useful. For most types, the following operations are done when parsing type `T`:
 - first, the object is parsed as `Any`: this is a very quick step, header is parsed, and object length is tested
 - next, DER constraint are tested for type `T` (if parsing DER)
 - finally, object is converted using `T::try_from(any)`. Other type-depdendant checks are done during this step.

## Examples and 

When writing a crate, the feature can be activated without changing the `Cargo.toml` file.
For example, if you want to run `example/print-cert` with trace enabled:
```shell
$ cargo run --features=asn1-rs/trace --example=print-cert -- ./assets/certificate.der
```