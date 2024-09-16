# Basics

This extension provides primitives for building authentication systems.

Currently supported authentication methods:

* [Password](password.md)

## Authentication Subject

Authentication subject is a provisional term for subject of authentication, such as user.

Note, however, that we treat it a bit more broadly. For example, an unrecognized identifier (such as login
or e-mail) can also be an authentication subject. This way we can track attempts to authenticate against
a non-existent user.

This can also be useful in the context where we are doing an authentication for a user that does not yet exist,
especially in the context of third-party OAuth authentications-as-signups.

## High-level Interface

### Authentication

`omni_auth.authenticate(Authenticator, authentication_subject_id)` function, dispatched over `Authenticator` types,
returns a value of an `Authentication` type that implements `omni_auth.successful_authentication` (see below)

Implementations:

* [Password](password.md#authenticating)

### Successful Authentication

`omni_auth.successful_authentication(Authentication)` returns a boolean that signifies success of authentication.

Implementation

* [Password](password.md#authenticating)


