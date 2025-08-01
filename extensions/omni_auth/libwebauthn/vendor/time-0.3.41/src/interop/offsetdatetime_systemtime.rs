use core::cmp::Ordering;
use core::ops::Sub;
use std::time::SystemTime;

use crate::{Duration, OffsetDateTime};

impl Sub<SystemTime> for OffsetDateTime {
    type Output = Duration;

    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    fn sub(self, rhs: SystemTime) -> Self::Output {
        self - Self::from(rhs)
    }
}

impl Sub<OffsetDateTime> for SystemTime {
    type Output = Duration;

    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    fn sub(self, rhs: OffsetDateTime) -> Self::Output {
        OffsetDateTime::from(self) - rhs
    }
}

impl PartialEq<SystemTime> for OffsetDateTime {
    fn eq(&self, rhs: &SystemTime) -> bool {
        self == &Self::from(*rhs)
    }
}

impl PartialEq<OffsetDateTime> for SystemTime {
    fn eq(&self, rhs: &OffsetDateTime) -> bool {
        &OffsetDateTime::from(*self) == rhs
    }
}

impl PartialOrd<SystemTime> for OffsetDateTime {
    fn partial_cmp(&self, other: &SystemTime) -> Option<Ordering> {
        self.partial_cmp(&Self::from(*other))
    }
}

impl PartialOrd<OffsetDateTime> for SystemTime {
    fn partial_cmp(&self, other: &OffsetDateTime) -> Option<Ordering> {
        OffsetDateTime::from(*self).partial_cmp(other)
    }
}

impl From<SystemTime> for OffsetDateTime {
    fn from(system_time: SystemTime) -> Self {
        match system_time.duration_since(SystemTime::UNIX_EPOCH) {
            Ok(duration) => Self::UNIX_EPOCH + duration,
            Err(err) => Self::UNIX_EPOCH - err.duration(),
        }
    }
}

impl From<OffsetDateTime> for SystemTime {
    fn from(datetime: OffsetDateTime) -> Self {
        let duration = datetime - OffsetDateTime::UNIX_EPOCH;

        if duration.is_zero() {
            Self::UNIX_EPOCH
        } else if duration.is_positive() {
            Self::UNIX_EPOCH + duration.unsigned_abs()
        } else {
            debug_assert!(duration.is_negative());
            Self::UNIX_EPOCH - duration.unsigned_abs()
        }
    }
}
