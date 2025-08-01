use crate::*;
#[cfg(not(feature = "std"))]
use alloc::vec::Vec;
use core::convert::TryFrom;
use core::fmt::Debug;

use self::debug::{macros::debug_eprintln, trace, trace_generic};

// // XXX this compiles but requires bound TryFrom :/
// impl<'a, 'b, T> TryFrom<&'b Any<'a>> for Vec<T>
// where
//     T: TryFrom<&'b Any<'a>>,
//     for<'e> <T as TryFrom<&'b Any<'a>>>::Error: From<Error>,
//     T: FromBer<'a, <T as TryFrom<&'b Any<'a>>>::Error>,
//     //     T: FromBer<'a, E>,
//     //     E: From<Error>,
// {
//     type Error = <T as TryFrom<&'b Any<'a>>>::Error;

//     fn try_from(any: &'b Any<'a>) -> Result<Vec<T>, Self::Error> {
//         any.tag().assert_eq(Self::TAG)?;
//         any.header.assert_constructed()?;
//         let v = SequenceIterator::<T, BerParser, Self::Error>::new(any.data)
//             .collect::<Result<Vec<T>, Self::Error>>()?;
//         Ok(v)
//     }
// }

// // XXX this compiles but requires bound TryFrom :/
// impl<'a, 'b, T> TryFrom<&'b Any<'a>> for Vec<T>
// where
//     T: TryFrom<&'b Any<'a>>,
//     <T as TryFrom<&'b Any<'a>>>::Error: From<Error>,
//     T: FromBer<'a, <T as TryFrom<&'b Any<'a>>>::Error>,
//     //     T: FromBer<'a, E>,
//     //     E: From<Error>,
// {
//     type Error = <T as TryFrom<&'b Any<'a>>>::Error;

//     fn try_from(any: &'b Any<'a>) -> Result<Vec<T>, Self::Error> {
//         any.tag().assert_eq(Self::TAG)?;
//         any.header.assert_constructed()?;
//         let v = SequenceIterator::<T, BerParser, Self::Error>::new(any.data)
//             .collect::<Result<Vec<T>, Self::Error>>()?;
//         Ok(v)
//     }
// }

impl<'a, T> TryFrom<Any<'a>> for Vec<T>
where
    T: FromBer<'a>,
{
    type Error = Error;

    fn try_from(any: Any<'a>) -> Result<Self> {
        trace_generic(
            core::any::type_name::<Self>(),
            "T::from(Any)",
            |any| {
                any.tag().assert_eq(Self::TAG)?;
                any.header.assert_constructed()?;
                let res_items: Result<Vec<T>> =
                    SetIterator::<T, BerParser>::new(any.data).collect();
                if res_items.is_err() {
                    debug_eprintln!(
                        core::any::type_name::<T>(),
                        "≠ {}",
                        "Conversion from Any failed".red()
                    );
                }
                res_items
            },
            any,
        )
    }
}

impl<T> CheckDerConstraints for Vec<T>
where
    T: CheckDerConstraints,
{
    fn check_constraints(any: &Any) -> Result<()> {
        any.tag().assert_eq(Self::TAG)?;
        any.header.assert_constructed()?;
        for item in SequenceIterator::<Any, DerParser>::new(any.data) {
            let item = item?;
            <T as CheckDerConstraints>::check_constraints(&item)?;
        }
        Ok(())
    }
}

impl<T> Tagged for Vec<T> {
    const TAG: Tag = Tag::Sequence;
}

// impl<'a, T> FromBer<'a> for Vec<T>
// where
//     T: FromBer<'a>,
// {
//     fn from_ber(bytes: &'a [u8]) -> ParseResult<Self> {
//         let (rem, any) = Any::from_ber(bytes)?;
//         any.header.assert_tag(Self::TAG)?;
//         let v = SequenceIterator::<T, BerParser>::new(any.data).collect::<Result<Vec<T>>>()?;
//         Ok((rem, v))
//     }
// }

/// manual impl of FromDer, so we do not need to require `TryFrom<Any> + CheckDerConstraints`
impl<'a, T, E> FromDer<'a, E> for Vec<T>
where
    T: FromDer<'a, E>,
    E: From<Error> + Debug,
{
    fn from_der(bytes: &'a [u8]) -> ParseResult<Self, E> {
        trace_generic(
            core::any::type_name::<Self>(),
            "Sequence::from_der",
            |bytes| {
                let (rem, any) = trace(core::any::type_name::<Self>(), parse_der_any, bytes)
                    .map_err(Err::convert)?;
                any.header
                    .assert_tag(Self::TAG)
                    .map_err(|e| Err::Error(e.into()))?;
                let v = SequenceIterator::<T, DerParser, E>::new(any.data)
                    .collect::<Result<Vec<T>, E>>()
                    .map_err(Err::Error)?;
                Ok((rem, v))
            },
            bytes,
        )
    }
}

#[cfg(feature = "std")]
impl<T> ToDer for Vec<T>
where
    T: ToDer,
{
    fn to_der_len(&self) -> Result<usize> {
        let mut len = 0;
        for t in self.iter() {
            len += t.to_der_len()?;
        }
        let header = Header::new(Class::Universal, true, Self::TAG, Length::Definite(len));
        Ok(header.to_der_len()? + len)
    }

    fn write_der_header(&self, writer: &mut dyn std::io::Write) -> SerializeResult<usize> {
        let mut len = 0;
        for t in self.iter() {
            len += t.to_der_len().map_err(|_| SerializeError::InvalidLength)?;
        }
        let header = Header::new(Class::Universal, true, Self::TAG, Length::Definite(len));
        header.write_der_header(writer).map_err(Into::into)
    }

    fn write_der_content(&self, writer: &mut dyn std::io::Write) -> SerializeResult<usize> {
        let mut sz = 0;
        for t in self.iter() {
            sz += t.write_der(writer)?;
        }
        Ok(sz)
    }
}
