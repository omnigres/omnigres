Webauthn Rust Proto Bindings
============================

Webauthn is a modern approach to hardware based authentication, consisting of
a user with an authenticator device, a browser or client that interacts with the
device, and a server that is able to generate challenges and verify the
authenticator's validity.

This crate contains the definitions used by Webauthn's server (Relying party, RP)
and the browser javascript (wasm) client components. In most cases you should
not need to interact with this library directly, opting instead to use the
implementations from [Webauthn-RS](https://docs.rs/webauthn-rs/) directly.
