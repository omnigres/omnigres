#![allow(unused_imports)]

use crate::ParseResult;

pub(crate) mod macros {
    macro_rules! debug_eprintln {
    ($msg: expr, $( $args:expr ),* ) => {
        #[cfg(feature = "debug")]
        {
            use colored::Colorize;
            let s = $msg.to_string().green();
            eprintln!("{} {}", s, format!($($args),*));
        }
    };
}

    #[allow(unused_macros)]
    macro_rules! trace_eprintln {
    ($msg: expr, $( $args:expr ),* ) => {
        #[cfg(feature = "trace")]
        {
            use colored::Colorize;
            let s = $msg.to_string().green();
            eprintln!("{} {}", s, format!($($args),*));
        }
    };
}

    pub(crate) use debug_eprintln;
    pub(crate) use trace_eprintln;
}

use macros::*;

#[cfg(feature = "debug")]
fn eprintln_hex_dump(bytes: &[u8], max_len: usize) {
    use core::cmp::min;
    use nom::HexDisplay;

    let m = min(bytes.len(), max_len);
    eprint!("{}", &bytes[..m].to_hex(16));
    if bytes.len() > max_len {
        eprintln!("... <continued>");
    }
}

#[cfg(not(feature = "debug"))]
#[inline]
pub fn trace_generic<F, I, O, E>(_msg: &str, _fname: &str, f: F, input: I) -> Result<O, E>
where
    F: Fn(I) -> Result<O, E>,
{
    f(input)
}

#[cfg(feature = "debug")]
pub fn trace_generic<F, I, O, E>(msg: &str, fname: &str, f: F, input: I) -> Result<O, E>
where
    F: Fn(I) -> Result<O, E>,
    E: core::fmt::Display,
{
    trace_eprintln!(msg, "⤷ {}", fname);
    let output = f(input);
    match &output {
        Err(e) => {
            debug_eprintln!(msg, "↯ {} failed: {}", fname, e.to_string().red());
        }
        _ => {
            debug_eprintln!(msg, "⤶ {}", fname);
        }
    }
    output
}

#[cfg(not(feature = "debug"))]
#[inline]
pub fn trace<'a, T, E, F>(_msg: &str, f: F, input: &'a [u8]) -> ParseResult<'a, T, E>
where
    F: Fn(&'a [u8]) -> ParseResult<'a, T, E>,
{
    f(input)
}

#[cfg(feature = "debug")]
pub fn trace<'a, T, E, F>(msg: &str, f: F, input: &'a [u8]) -> ParseResult<'a, T, E>
where
    F: Fn(&'a [u8]) -> ParseResult<'a, T, E>,
{
    trace_eprintln!(
        msg,
        "⤷ input (len={}, type={})",
        input.len(),
        core::any::type_name::<T>()
    );
    let res = f(input);
    match &res {
        Ok((_rem, _)) => {
            trace_eprintln!(
                msg,
                "⤶ Parsed {} bytes, {} remaining",
                input.len() - _rem.len(),
                _rem.len()
            );
        }
        Err(_) => {
            // NOTE: we do not need to print error, caller should print it
            debug_eprintln!(msg, "↯ Parsing failed at location:");
            eprintln_hex_dump(input, 16);
        }
    }
    res
}

#[cfg(feature = "debug")]
#[cfg(test)]
mod tests {

    use std::collections::HashSet;

    use crate::*;
    use alloc::collections::BTreeSet;
    use hex_literal::hex;

    #[test]
    fn debug_from_ber_any() {
        assert!(Any::from_ber(&hex!("01 01 ff")).is_ok());
    }

    #[test]
    fn debug_from_ber_failures() {
        // wrong type
        eprintln!("--");
        assert!(<Vec<u16>>::from_ber(&hex!("02 01 00")).is_err());
    }

    #[test]
    fn debug_from_ber_sequence_indefinite() {
        let input = &hex!("30 80 02 03 01 00 01 00 00");
        let (rem, result) = Sequence::from_ber(input).expect("parsing failed");
        assert_eq!(result.as_ref(), &input[2..7]);
        assert_eq!(rem, &[]);
        eprintln!("--");
        let (rem, result) = <Vec<u32>>::from_ber(input).expect("parsing failed");
        assert_eq!(&result, &[65537]);
        assert_eq!(rem, &[]);
    }

    #[test]
    fn debug_from_ber_sequence_of() {
        // parsing failure (wrong type)
        let input = &hex!("30 03 01 01 00");
        eprintln!("--");
        let _ = <SequenceOf<u32>>::from_ber(input).expect_err("parsing should fail");
        eprintln!("--");
        let _ = <Vec<u32>>::from_ber(input).expect_err("parsing should fail");
    }

    #[test]
    fn debug_from_ber_u32() {
        assert!(u32::from_ber(&hex!("02 01 01")).is_ok());
    }

    #[test]
    fn debug_from_der_any() {
        assert!(Any::from_der(&hex!("01 01 ff")).is_ok());
    }

    #[test]
    fn debug_from_der_bool() {
        eprintln!("** first test is ok**");
        assert!(<bool>::from_der(&hex!("01 01 ff")).is_ok());
        eprintln!("** second test fails when parsing ANY (eof)**");
        assert!(<bool>::from_der(&hex!("01 02 ff")).is_err());
        eprintln!("** second test fails when checking DER constraints**");
        assert!(<bool>::from_der(&hex!("01 01 f0")).is_err());
        eprintln!("** second test fails during TryFrom**");
        assert!(<bool>::from_der(&hex!("01 02 ff ff")).is_err());
    }

    #[test]
    fn debug_from_der_failures() {
        use crate::Sequence;

        // parsing any failed
        eprintln!("--");
        assert!(u16::from_der(&hex!("ff 00")).is_err());
        // Indefinite length
        eprintln!("--");
        assert!(u16::from_der(&hex!("30 80 00 00")).is_err());
        // DER constraints failed
        eprintln!("--");
        assert!(bool::from_der(&hex!("01 01 7f")).is_err());
        // Incomplete sequence
        eprintln!("--");
        let _ = Sequence::from_der(&hex!("30 81 04 00 00"));
    }

    #[test]
    fn debug_from_der_sequence() {
        // parsing OK, recursive
        let input = &hex!("30 08 02 03 01 00 01 02 01 01");
        let (rem, result) = <Vec<u32>>::from_der(input).expect("parsing failed");
        assert_eq!(&result, &[65537, 1]);
        assert_eq!(rem, &[]);
    }

    #[test]
    fn debug_from_der_sequence_fail() {
        // tag is wrong
        let input = &hex!("31 03 01 01 44");
        let _ = <Vec<bool>>::from_der(input).expect_err("parsing should fail");
        // sequence is ok but contraint fails on element
        let input = &hex!("30 03 01 01 44");
        let _ = <Vec<bool>>::from_der(input).expect_err("parsing should fail");
    }

    #[test]
    fn debug_from_der_sequence_of() {
        use crate::SequenceOf;
        // parsing failure (wrong type)
        let input = &hex!("30 03 01 01 00");
        eprintln!("--");
        let _ = <SequenceOf<u32>>::from_der(input).expect_err("parsing should fail");
        eprintln!("--");
        let _ = <Vec<u32>>::from_der(input).expect_err("parsing should fail");
    }

    #[test]
    fn debug_from_der_set_fail() {
        // set is ok but contraint fails on element
        let input = &hex!("31 03 01 01 44");
        let _ = <BTreeSet<bool>>::from_der(input).expect_err("parsing should fail");
    }

    #[test]
    fn debug_from_der_set_of() {
        use crate::SetOf;
        use alloc::collections::BTreeSet;

        // parsing failure (wrong type)
        let input = &hex!("31 03 01 01 00");
        eprintln!("--");
        let _ = <SetOf<u32>>::from_der(input).expect_err("parsing should fail");
        eprintln!("--");
        let _ = <BTreeSet<u32>>::from_der(input).expect_err("parsing should fail");
        eprintln!("--");
        let _ = <HashSet<u32>>::from_der(input).expect_err("parsing should fail");
    }

    #[test]
    fn debug_from_der_string_ok() {
        let input = &hex!("0c 0a 53 6f 6d 65 2d 53 74 61 74 65");
        let (rem, result) = Utf8String::from_der(input).expect("parsing failed");
        assert_eq!(result.as_ref(), "Some-State");
        assert_eq!(rem, &[]);
    }

    #[test]
    fn debug_from_der_string_fail() {
        // wrong charset
        let input = &hex!("12 03 41 42 43");
        let _ = NumericString::from_der(input).expect_err("parsing should fail");
    }
}
