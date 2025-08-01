use std::env;

const OPENSSL_DOC: &str = "https://github.com/kanidm/webauthn-rs/blob/master/OpenSSL.md";

fn main() {
    println!();
    if let Ok(v) = env::var("DEP_OPENSSL_VERSION_NUMBER") {
        let version =
            u64::from_str_radix(&v, 16).expect("Failed to parse OpenSSL version in build.rs");
        #[allow(clippy::unusual_byte_groupings)]
        if version >= 0x3_00_00_00_0 {
            return;
        } else {
            println!(
                r#"
Your version of OpenSSL is out of date, and not supported by this library.

Please upgrade to OpenSSL v3.0.0 or later.

More info: {OPENSSL_DOC}
OpenSSL version string: {version}
"#
            );
            panic!("The installed version of OpenSSL is unusable.");
        }
    }

    panic!("No version of OpenSSL is found.");
}
