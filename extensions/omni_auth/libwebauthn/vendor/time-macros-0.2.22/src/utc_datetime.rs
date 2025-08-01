use std::iter::Peekable;

use proc_macro::{token_stream, TokenTree};

use crate::date::Date;
use crate::error::Error;
use crate::time::Time;
use crate::to_tokens::ToTokenTree;
use crate::{date, time};

pub(crate) struct UtcDateTime {
    date: Date,
    time: Time,
}

pub(crate) fn parse(chars: &mut Peekable<token_stream::IntoIter>) -> Result<UtcDateTime, Error> {
    let date = date::parse(chars)?;
    let time = time::parse(chars)?;

    if let Some(token) = chars.peek() {
        return Err(Error::UnexpectedToken {
            tree: token.clone(),
        });
    }

    Ok(UtcDateTime { date, time })
}

impl ToTokenTree for UtcDateTime {
    fn into_token_tree(self) -> TokenTree {
        quote_group! {{
            const DATE_TIME: ::time::UtcDateTime = ::time::UtcDateTime::new(
                #(self.date),
                #(self.time),
            );
            DATE_TIME
        }}
    }
}
