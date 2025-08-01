use crate::*;
#[cfg(not(feature = "std"))]
use alloc::vec::Vec;
use core::convert::TryFrom;
use core::fmt::{Debug, Display};
use core::iter::FromIterator;
use core::ops::{Deref, DerefMut};

use self::debug::{trace, trace_generic};

/// The `SET OF` object is an unordered list of homogeneous types.
///
/// This type implements `Deref<Target = [T]>` and `DerefMut<Target = [T]>`, so all methods
/// like `.iter()`, `.len()` and others can be used transparently as if using a vector.
///
/// # Examples
///
/// ```
/// use asn1_rs::SetOf;
/// use std::iter::FromIterator;
///
/// // build set
/// let set = SetOf::from_iter([2, 3, 4]);
///
/// // `set` now contains the serialized DER representation of the array
///
/// // iterate objects
/// let mut sum = 0;
/// for item in set.iter() {
///     // item has type `Result<u32>`, since parsing the serialized bytes could fail
///     sum += *item;
/// }
/// assert_eq!(sum, 9);
///
/// ```
#[derive(Debug, PartialEq)]
pub struct SetOf<T> {
    items: Vec<T>,
}

impl<T> SetOf<T> {
    /// Builds a `SET OF` from the provided content
    #[inline]
    pub const fn new(items: Vec<T>) -> Self {
        SetOf { items }
    }

    /// Converts `self` into a vector without clones or allocation.
    #[inline]
    pub fn into_vec(self) -> Vec<T> {
        self.items
    }

    /// Appends an element to the back of a collection
    #[inline]
    pub fn push(&mut self, item: T) {
        self.items.push(item)
    }
}

impl<T> AsRef<[T]> for SetOf<T> {
    fn as_ref(&self) -> &[T] {
        &self.items
    }
}

impl<T> AsMut<[T]> for SetOf<T> {
    fn as_mut(&mut self) -> &mut [T] {
        &mut self.items
    }
}

impl<T> Deref for SetOf<T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        &self.items
    }
}

impl<T> DerefMut for SetOf<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.items
    }
}

impl<T> From<SetOf<T>> for Vec<T> {
    fn from(set: SetOf<T>) -> Self {
        set.items
    }
}

impl<T> FromIterator<T> for SetOf<T> {
    fn from_iter<IT: IntoIterator<Item = T>>(iter: IT) -> Self {
        let items = Vec::from_iter(iter);
        SetOf::new(items)
    }
}

impl<'a, T> TryFrom<Any<'a>> for SetOf<T>
where
    T: FromBer<'a>,
{
    type Error = Error;

    fn try_from(any: Any<'a>) -> Result<Self> {
        any.tag().assert_eq(Self::TAG)?;
        if !any.header.is_constructed() {
            return Err(Error::ConstructExpected);
        }
        let items = SetIterator::<T, BerParser>::new(any.data).collect::<Result<Vec<T>>>()?;
        Ok(SetOf::new(items))
    }
}

impl<T> CheckDerConstraints for SetOf<T>
where
    T: CheckDerConstraints,
{
    fn check_constraints(any: &Any) -> Result<()> {
        any.tag().assert_eq(Self::TAG)?;
        any.header.assert_constructed()?;
        for item in SetIterator::<Any, DerParser>::new(any.data) {
            let item = item?;
            T::check_constraints(&item)?;
        }
        Ok(())
    }
}

/// manual impl of FromDer, so we do not need to require `TryFrom<Any> + CheckDerConstraints`
impl<'a, T, E> FromDer<'a, E> for SetOf<T>
where
    T: FromDer<'a, E>,
    E: From<Error> + Display + Debug,
{
    fn from_der(bytes: &'a [u8]) -> ParseResult<Self, E> {
        trace_generic(
            core::any::type_name::<Self>(),
            "SetOf::from_der",
            |bytes| {
                let (rem, any) = trace(core::any::type_name::<Self>(), Any::from_der, bytes)
                    .map_err(Err::convert)?;
                any.header
                    .assert_tag(Self::TAG)
                    .map_err(|e| Err::Error(e.into()))?;
                let items = SetIterator::<T, DerParser, E>::new(any.data)
                    .collect::<Result<Vec<T>, E>>()
                    .map_err(Err::Error)?;
                Ok((rem, SetOf::new(items)))
            },
            bytes,
        )
    }
}

impl<T> Tagged for SetOf<T> {
    const TAG: Tag = Tag::Set;
}

#[cfg(feature = "std")]
impl<T> ToDer for SetOf<T>
where
    T: ToDer,
{
    fn to_der_len(&self) -> Result<usize> {
        self.items.to_der_len()
    }

    fn write_der_header(&self, writer: &mut dyn std::io::Write) -> SerializeResult<usize> {
        // Do not call self.items.write_der_header(), this will encode the wrong tag (items is a Vec)
        let mut len = 0;
        for t in self.items.iter() {
            len += t.to_der_len().map_err(|_| SerializeError::InvalidLength)?;
        }
        let header = Header::new(Class::Universal, true, Self::TAG, Length::Definite(len));
        header.write_der_header(writer).map_err(Into::into)
    }

    fn write_der_content(&self, writer: &mut dyn std::io::Write) -> SerializeResult<usize> {
        self.items.write_der_content(writer)
    }
}
