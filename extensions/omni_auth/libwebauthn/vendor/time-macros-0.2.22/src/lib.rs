#![allow(
    clippy::missing_const_for_fn, // irrelevant for proc macros
    clippy::missing_docs_in_private_items, // TODO remove
    clippy::std_instead_of_core, // irrelevant for proc macros
    clippy::std_instead_of_alloc, // irrelevant for proc macros
    clippy::alloc_instead_of_core, // irrelevant for proc macros
    missing_docs, // TODO remove
)]

#[allow(unused_macros)]
macro_rules! bug {
    () => { compile_error!("provide an error message to help fix a possible bug") };
    ($descr:literal $($rest:tt)?) => {
        unreachable!(concat!("internal error: ", $descr) $($rest)?)
    }
}

#[macro_use]
mod quote;

mod date;
mod datetime;
mod error;
#[cfg(any(feature = "formatting", feature = "parsing"))]
mod format_description;
mod helpers;
mod offset;
#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
mod serde_format_description;
mod time;
mod to_tokens;
mod utc_datetime;

#[cfg(any(feature = "formatting", feature = "parsing"))]
use std::iter::Peekable;

#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
use proc_macro::Delimiter;
use proc_macro::TokenStream;
#[cfg(any(feature = "formatting", feature = "parsing"))]
use proc_macro::TokenTree;

use self::error::Error;

macro_rules! impl_macros {
    ($($name:ident)*) => {$(
        #[proc_macro]
        pub fn $name(input: TokenStream) -> TokenStream {
            use crate::to_tokens::ToTokenTree;

            let mut iter = input.into_iter().peekable();
            match $name::parse(&mut iter) {
                Ok(value) => match iter.peek() {
                    Some(tree) => Error::UnexpectedToken { tree: tree.clone() }.to_compile_error(),
                    None => TokenStream::from(value.into_token_tree()),
                },
                Err(err) => err.to_compile_error(),
            }
        }
    )*};
}

impl_macros![date datetime utc_datetime offset time];

#[cfg(any(feature = "formatting", feature = "parsing"))]
type PeekableTokenStreamIter = Peekable<proc_macro::token_stream::IntoIter>;

#[cfg(any(feature = "formatting", feature = "parsing"))]
enum FormatDescriptionVersion {
    V1,
    V2,
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
fn parse_format_description_version<const NO_EQUALS_IS_MOD_NAME: bool>(
    iter: &mut PeekableTokenStreamIter,
) -> Result<Option<FormatDescriptionVersion>, Error> {
    let end_of_input_err = || {
        if NO_EQUALS_IS_MOD_NAME {
            Error::UnexpectedEndOfInput
        } else {
            Error::ExpectedString {
                span_start: None,
                span_end: None,
            }
        }
    };
    let version_ident = match iter.peek().ok_or_else(end_of_input_err)? {
        version @ TokenTree::Ident(ident) if ident.to_string() == "version" => {
            let version_ident = version.clone();
            iter.next(); // consume `version`
            version_ident
        }
        _ => return Ok(None),
    };

    match iter.peek() {
        Some(TokenTree::Punct(punct)) if punct.as_char() == '=' => iter.next(),
        _ if NO_EQUALS_IS_MOD_NAME => {
            // Push the `version` ident to the front of the iterator.
            *iter = std::iter::once(version_ident)
                .chain(iter.clone())
                .collect::<TokenStream>()
                .into_iter()
                .peekable();
            return Ok(None);
        }
        Some(token) => {
            return Err(Error::Custom {
                message: "expected `=`".into(),
                span_start: Some(token.span()),
                span_end: Some(token.span()),
            });
        }
        None => {
            return Err(Error::Custom {
                message: "expected `=`".into(),
                span_start: None,
                span_end: None,
            });
        }
    };
    let version_literal = match iter.next() {
        Some(TokenTree::Literal(literal)) => literal,
        Some(token) => {
            return Err(Error::Custom {
                message: "expected 1 or 2".into(),
                span_start: Some(token.span()),
                span_end: Some(token.span()),
            });
        }
        None => {
            return Err(Error::Custom {
                message: "expected 1 or 2".into(),
                span_start: None,
                span_end: None,
            });
        }
    };
    let version = match version_literal.to_string().as_str() {
        "1" => FormatDescriptionVersion::V1,
        "2" => FormatDescriptionVersion::V2,
        _ => {
            return Err(Error::Custom {
                message: "invalid format description version".into(),
                span_start: Some(version_literal.span()),
                span_end: Some(version_literal.span()),
            });
        }
    };
    helpers::consume_punct(',', iter)?;

    Ok(Some(version))
}

#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
fn parse_visibility(iter: &mut PeekableTokenStreamIter) -> Result<TokenStream, Error> {
    let mut visibility = match iter.peek().ok_or(Error::UnexpectedEndOfInput)? {
        pub_ident @ TokenTree::Ident(ident) if ident.to_string() == "pub" => {
            let visibility = quote! { #(pub_ident.clone()) };
            iter.next(); // consume `pub`
            visibility
        }
        _ => return Ok(quote! {}),
    };

    match iter.peek().ok_or(Error::UnexpectedEndOfInput)? {
        group @ TokenTree::Group(path) if path.delimiter() == Delimiter::Parenthesis => {
            visibility.extend(std::iter::once(group.clone()));
            iter.next(); // consume parentheses and path
        }
        _ => {}
    }

    Ok(visibility)
}

#[cfg(any(feature = "formatting", feature = "parsing"))]
#[proc_macro]
pub fn format_description(input: TokenStream) -> TokenStream {
    (|| {
        let mut input = input.into_iter().peekable();
        let version = parse_format_description_version::<false>(&mut input)?;
        let (span, string) = helpers::get_string_literal(input)?;
        let items = format_description::parse_with_version(version, &string, span)?;

        Ok(quote! {{
            const DESCRIPTION: &[::time::format_description::BorrowedFormatItem<'_>] = &[#S(
                items
                    .into_iter()
                    .map(|item| quote! { #S(item), })
                    .collect::<TokenStream>()
            )];
            DESCRIPTION
        }})
    })()
    .unwrap_or_else(|err: Error| err.to_compile_error())
}

#[cfg(all(feature = "serde", any(feature = "formatting", feature = "parsing")))]
#[proc_macro]
pub fn serde_format_description(input: TokenStream) -> TokenStream {
    (|| {
        let mut tokens = input.into_iter().peekable();

        // First, the optional format description version.
        let version = parse_format_description_version::<true>(&mut tokens)?;

        // Then, the visibility of the module.
        let visibility = parse_visibility(&mut tokens)?;

        // Next, an identifier (the desired module name)
        let mod_name = match tokens.next() {
            Some(TokenTree::Ident(ident)) => Ok(ident),
            Some(tree) => Err(Error::UnexpectedToken { tree }),
            None => Err(Error::UnexpectedEndOfInput),
        }?;

        // Followed by a comma
        helpers::consume_punct(',', &mut tokens)?;

        // Then, the type to create serde serializers for (e.g., `OffsetDateTime`).
        let formattable = match tokens.next() {
            Some(tree @ TokenTree::Ident(_)) => Ok(tree),
            Some(tree) => Err(Error::UnexpectedToken { tree }),
            None => Err(Error::UnexpectedEndOfInput),
        }?;

        // Another comma
        helpers::consume_punct(',', &mut tokens)?;

        // We now have two options. The user can either provide a format description as a string or
        // they can provide a path to a format description. If the latter, all remaining tokens are
        // assumed to be part of the path.
        let (format, format_description_display) = match tokens.peek() {
            // string literal
            Some(TokenTree::Literal(_)) => {
                let (span, format_string) = helpers::get_string_literal(tokens)?;
                let items = format_description::parse_with_version(version, &format_string, span)?;
                let items: TokenStream =
                    items.into_iter().map(|item| quote! { #S(item), }).collect();
                let items = quote! {
                    const ITEMS: &[::time::format_description::BorrowedFormatItem<'_>]
                        = &[#S(items)];
                    ITEMS
                };

                (items, String::from_utf8_lossy(&format_string).into_owned())
            }
            // path
            Some(_) => {
                let tokens = tokens.collect::<TokenStream>();
                let tokens_string = tokens.to_string();
                (tokens, tokens_string)
            }
            None => return Err(Error::UnexpectedEndOfInput),
        };

        Ok(serde_format_description::build(
            visibility,
            mod_name,
            formattable,
            format,
            format_description_display,
        ))
    })()
    .unwrap_or_else(|err: Error| err.to_compile_error_standalone())
}
