use num_conv::prelude::*;

use crate::convert::*;
use crate::OffsetDateTime;

impl From<js_sys::Date> for OffsetDateTime {
    /// # Panics
    ///
    /// This may panic if the timestamp can not be represented.
    fn from(js_date: js_sys::Date) -> Self {
        // get_time() returns milliseconds
        let timestamp_nanos = js_date.get_time() as i128
            * Nanosecond::per(Millisecond).cast_signed().extend::<i128>();
        Self::from_unix_timestamp_nanos(timestamp_nanos)
            .expect("invalid timestamp: Timestamp cannot fit in range")
    }
}

impl From<OffsetDateTime> for js_sys::Date {
    fn from(datetime: OffsetDateTime) -> Self {
        // new Date() takes milliseconds
        let timestamp = (datetime.unix_timestamp_nanos()
            / Nanosecond::per(Millisecond).cast_signed().extend::<i128>())
            as f64;
        Self::new(&timestamp.into())
    }
}
