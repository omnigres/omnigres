Webauthn Rust Core
==================

Webauthn is a modern approach to hardware based authentication, consisting of
a user with an authenticator device, a browser or client that interacts with the
device, and a server that is able to generate challenges and verify the
authenticator's validity.

⚠️  WARNING ⚠️
-------------

This library implements and exposes the *raw* elements to create a Webauthn Relying
Party. Many of these components have many sharp edges and the ability to confuse
users, accidentally allow security bypasses, and more. If possible you SHOULD use
[Webauthn-RS](https://docs.rs/webauthn-rs/) instead of this crate!

However, if you want to do something truly custom or specific, and you understand the
risks, then this library is for you.

Why OpenSSL?
------------

A question I expect is why OpenSSL rather than some other pure-Rust cryptographic
providers. There are two major justfications.

The first is that if this library will be used in corporate or major deployments,
then cryptographic audits may have to be performed. It is much easier to point
toward OpenSSL which has already undergone much more review and auditing than
using a series of Rust crates which (while still great!) have not seen the same
level of scrutiny.

The second is that OpenSSL is the only library I have found that allows us to
reconstruct an EC public key from its X/Y points or an RSA public key from its
n/e for use with signature verification.
Without this, we are not able to parse authenticator credentials to perform authentication.

Resources
---------

* Specification: https://www.w3.org/TR/webauthn-3
* JSON details: https://fidoalliance.org/specs/fido-v2.0-rd-20180702/fido-server-v2.0-rd-20180702.html
* Write up on interactions: https://medium.com/@herrjemand/introduction-to-webauthn-api-5fd1fb46c285



