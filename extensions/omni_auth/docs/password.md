# Password Authentication

Features:

* Hash-based verification (**bcrypt**, _more algorithms pending_)
* Password authentication attempt audit trail
* Temporal password management

## Password Credentials

`omni_auth.password_credentials` table is a temporally-enabled table that contains hashed passwords
for [authentication subjects](basics.md#authentication-subject).

For every given `authentication_subject_id` there may be _zero_ or _one_ `hashed_password` for any non-overlapping
timestamp-period. The latest valid value is denoted by `valid_at` that includes current timestamp.

## Setting a password

Considering the temporality of passwords, `omni_auth` provides a function that encapsulates the complexity of managing
passwords. It will:

* create a new password if none is available
* make "current" password a "historic" one by setting its validity until the validity of the new one
* validate that the old password is matching, if supplied
* ensure that the new password has the same upper bound of validity as the "current" one
* allow to specify the validity period for the new password explicitly

The most common scenario is to set a password

```postgresql
select omni_auth.set_password(authentication_subject_id, password, [old_password])
```

|                     Parameter | Type                                | Description                                                                               |
|------------------------------:|-------------------------------------|-------------------------------------------------------------------------------------------|
| **authentication_subject_id** | omni_auth.authentication_subject_id | Authentication Subject ID to set password for                                             |
|                  **password** | omni_auth.password                  | New password to set                                                                       |
|              **old_password** | omni_auth.password                  | Old password to check against (optional)                                                  |
|                **valid_from** | timestamptz                         | New password should be valid from, inclusive (optional)                                   |
|               **valid_until** | timestamptz                         | New password should be valid until, exclusive (optional, default `statement_timestamp()`) |
|         **hashing_algorithm** | omni_auth.hashing_algorithm         | Hashing algorithm for the new password (optional, using the default one)                  |
|               **work_factor** | int                                 | Hashing algorithm work factor (optional)                                                  |

## Authenticating

To attempt authentication with a given password for an authentication subject, use the following function

```postgresql
select omni_auth.authenticate(password, authentication_subject_id, [as_of])
```

It will return a record of the `omni_auth.password_authentications` type, which can be verified for success using
[`omni_auth.successful_authentication()`](basics.md#successful-authentication).

!!! tip "Temporal authentication"

    `as_of` parameter (of `timestamptz` type) can be used to authenticate against a password that could have been
    available at that point in time.

## Hashed Password

`omni_auth.hash_password` provides a facility to create values of the `omni_auth.hashed_password` type which is used
in the [`omni_auth.password_credentials` table](#password-credentials). This is typically not needed if [
`omni_auth.set_password`](#setting-a-password) is used.

### Work Factor Calibration

OWASP recommends that the hashing function takes about a second for a balance of usability and security aspects.
However, on different computers, different work factors may result in different timing. To address this, `omni_auth`
provides a materialized view `omni_auth.password_work_factor_timings` (unpopulated at first) that will provide timings
for supported algorithms for different work factors (by default capped at 1.5 seconds).

`omni_auth` attempts to set sensible defaults in absence of populated data in `omni_auth.password_work_factor_timings`,
but it can be modified using the following variables:

|                    Variable name | Description                                 |
|---------------------------------:|---------------------------------------------|
| **omni_auth.bcrypt_work_factor** | **bcrypt** work factor (defaults to **12**) |