//! Fake `CredentialID` generator. See [WebauthnFakeCredentialGenerator] for more details.

use openssl::{hash, pkey, sign};
use rand::prelude::*;
use rand_chacha::ChaCha8Rng;

use crate::error::WebauthnError;
use crate::proto::CredentialID;

/// A trait for implementing custom `CredentialID` distributions. You *must* use the provided
/// rng for generating `CredentialID`s to ensure that the outputs are deterministic.
pub trait FakeCredentialIDDistribution {
    /// Given the provided rng, generate fake `CredentialID`s.
    fn generate<R: RngCore>(seeded_rng: &mut R) -> Vec<CredentialID>;
}

// Median number of credentials on an account. Due to synced credentials a lot of consumers will
// only have a single credential contrast to security key users who tend to use 2 or more.
const CREDENTIAL_MEDIAN: u8 = 1;

// Various credential type lengths, taken from observation.

/// Size of the `CredentialID` returned by a Yubikey Series 5 with ECDSA.
pub const YUBIKEY_CRED_LEN: usize = 64;
/// Size of the `CredentialID` returned by a Yubikey Series 5 with EDDSA.
pub const YUBIKEY_EDDSA_CRED_LEN: usize = 128;
/// Size of the `CredentialID` returned by a TPM, including Windows Hello.
pub const TPM_CRED_LEN: usize = 32;
/// Size of the `CredentialID` returned by a Google Pixel.
pub const G_PIXEL_CRED_LEN: usize = 65;
/// Size of the `CredentialID` returned by Apple Keychain on MacOS and iOS.
pub const APPLE_CRED_LEN: usize = 20;
/// Size of the `CredentialID` returned by Bitwarden Password Manager
pub const BITWARDEN_CRED_LEN: usize = 16;

// Solokeys EDDSA.
// const SK_EDDSA_CRED_LEN_1: usize = 268;
// const SK_EDDSA_CRED_LEN_1: usize = 211;
// const SK_EDDSA_CRED_LEN_1: usize = 195;

// These are the values for std distribution of a u32. We use this for
// helping create believable ranges of credential's on accounts etc.
const SD_02: u32 = 94489281;
const SD_15: u32 = 678604833;
// const SD_50: u32 = 2147483647;
const SD_84: u32 = 3612067496;
const SD_98: u32 = 4200478015;

// =================================================

// These numbers are open to discussion.

// If you only have one cred, we want to "fake" a synced credential. Today that's most likely
// an iphone or android, which is split 70/30 android/ios. We also want to consider third
// party password managers - these tend to be only used by tech communities rather than the
// general population. then factor this so that 98% of these are mobile keychains, 2% are
// pw managers

const CRED_SINGLE_68: u32 = 2946347564;
const CRED_SINGLE_98: u32 = 4200478014;

// For multiple credentials, we assume a broader range of possible credentials.
// https://gs.statcounter.com/os-market-share/
//
// Android  41.64%  - g-pixel
// Windows  29.25%  - tpm
// iOS      17.71%  - apple kc
// OS X     6.57%   - apple kc
// Unknown  1.93%   - n/a
// Linux    1.54%   - n/a
//
// Similar to above, we assume that pw managers and security keys are limited in the
// general consumer populace. For now I'm going to assume this at about 4% as an
// arbitrary number. Especially for unknown + linux, these rely on sk/pw manager anyway
//
// I'm also going to reduce the windows tpm values to 1/4 - windows hello probably has
// less adoption than anything else, both due to tpm requirements, but also general
// consumer rejection of ms related tech. This means we should distribute and inflate the
// other values.
//
// With that done, we end up with roughly.
//
// Android  50.61%  - g-pixel
// Windows   8.68%  - tpm
// Apl      33.25%  - apple kc
// Other     7.46%  - n/a
//
// Within "other" we assume:
//
// 50% pw manager
// 50% yk

const CRED_MULTI_ANDROID: u32 = 2173682947;
const CRED_MULTI_WINDOWS: u32 = CRED_MULTI_ANDROID + 372803161;
const CRED_MULTI_APL: u32 = CRED_MULTI_WINDOWS + 1428076625;
const CRED_MULTI_YK: u32 = CRED_MULTI_APL + 160202280;

/// A distribution for service providers which have passkeys as an authentication option parallel
/// to passwords and other MFA types. This type assumes that not all users may have registered
/// passkeys to their accounts.
pub struct FakePasskeyDistribution;

impl FakeCredentialIDDistribution for FakePasskeyDistribution {
    /// Given the provided rng, generate fake `CredentialID`s.
    fn generate<R: RngCore>(seeded_rng: &mut R) -> Vec<CredentialID> {
        // How many credentials should we create?
        let cred_dist = seeded_rng.next_u32();

        let creds_to_generate = if cred_dist < SD_02 {
            // -2
            CREDENTIAL_MEDIAN.saturating_sub(2)
        } else if cred_dist < SD_15 {
            // -1
            CREDENTIAL_MEDIAN.saturating_sub(1)
        } else if cred_dist < SD_84 {
            // -0
            CREDENTIAL_MEDIAN
        } else if cred_dist < SD_98 {
            // +1
            CREDENTIAL_MEDIAN.saturating_add(1)
        } else {
            // +2
            CREDENTIAL_MEDIAN.saturating_add(2)
        };

        let mut credentials = Vec::with_capacity(creds_to_generate as usize);

        if creds_to_generate == 1 {
            let type_dist = seeded_rng.next_u32();

            let cred_len = if type_dist < CRED_SINGLE_68 {
                // Android
                G_PIXEL_CRED_LEN
            } else if type_dist < CRED_SINGLE_98 {
                // iOS
                APPLE_CRED_LEN
            } else {
                // pw manager
                BITWARDEN_CRED_LEN
            };

            let mut cred = vec![0; cred_len];
            seeded_rng.fill_bytes(&mut cred);

            credentials.push(cred.into());
        } else {
            for _i in 0..creds_to_generate {
                let type_dist = seeded_rng.next_u32();

                let cred_len = if type_dist < CRED_MULTI_ANDROID {
                    // Android
                    G_PIXEL_CRED_LEN
                } else if type_dist < CRED_MULTI_WINDOWS {
                    // Windows
                    TPM_CRED_LEN
                } else if type_dist < CRED_MULTI_APL {
                    // Apple
                    APPLE_CRED_LEN
                } else if type_dist < CRED_MULTI_YK {
                    // Other - yk
                    YUBIKEY_CRED_LEN
                } else {
                    BITWARDEN_CRED_LEN
                };

                let mut cred = vec![0; cred_len];
                seeded_rng.fill_bytes(&mut cred);

                credentials.push(cred.into());
            }
        }

        credentials
    }
}

/// A fake `CredentialID` generator. This allows RP's to implement account enumeration defences if
/// they so choose. Since webauthn requires the RP to send `CredentialID`s in challenges, when a
/// user does not exist this will either provide an error or an empty list of IDs. This becomes a
/// signal that an account with IDs associated to it in challenges must exist.
///
/// Account enumeration defence is a very subjective matter. For example on a website for a
/// psychologists or drug addiction service, account enumeration prevention is important for
/// privacy of the users of the service. For a service such as a business it may be less important
/// as corporate directories may be available.
///
/// Because of this some considerations have been made in this api.
///
/// The only input we have from the account enumeration is the username.
///
/// Two users of this library should not have the same generated credentials given the same username.
///
/// `CredentialID`s that are generated should be believable and appear to come from legitimate services
/// or credential sources. This includes realistic distribution of the types of devices that users
/// may be using.
///
/// This api must be consistent - the same inputs will yield the same outputs.
///
/// To achieve this, the generator uses a seeded CSPRNG for each operation. The CSPRNG is seeded
/// from the HMAC of the username. The HMAC is keyed from an input that the site provides.
///
/// The HMAC key MUST NOT be disclosed, as knowledge of the HMAC key will allow an external
/// party to determine which IDs are generated and which are not.
pub struct WebauthnFakeCredentialGenerator<D>
where
    D: FakeCredentialIDDistribution,
{
    // hmac key
    hmac_key: pkey::PKey<pkey::Private>,
    distribution: std::marker::PhantomData<D>,
}

impl<D> WebauthnFakeCredentialGenerator<D>
where
    D: FakeCredentialIDDistribution,
{
    /// Generate a new random HMAC key for the credential generator. You MUST persist this key for
    /// future use.
    pub fn new_hmac_key() -> Result<Vec<u8>, WebauthnError> {
        let mut key = vec![0; 16];

        openssl::rand::rand_bytes(&mut key)
            .map_err(WebauthnError::OpenSSLError)
            .map(|_| key)
    }
}

impl<D> WebauthnFakeCredentialGenerator<D>
where
    D: FakeCredentialIDDistribution,
{
    /// Create a new HMAC keyed fake credential generator. You should associate a distribution type
    /// using type annotations. The `hmac_key` is a secret value. Disclosure of it will allow an
    /// external party to determine if `CredentialID`s are genuine or faked. Rotation of this key
    /// may also allow detection of genuine or fake credentials.
    pub fn new(hmac_key: &[u8]) -> Result<Self, WebauthnError> {
        let hmac_key = pkey::PKey::hmac(hmac_key).map_err(WebauthnError::OpenSSLError)?;

        Ok(WebauthnFakeCredentialGenerator {
            hmac_key,
            distribution: std::marker::PhantomData,
        })
    }

    /// Given a username as a byte slice, generate a set of deterministic `CredentialID`s.
    pub fn generate(&self, username: &[u8]) -> Result<Vec<CredentialID>, WebauthnError> {
        // hmac the username
        let mut signer = sign::Signer::new(hash::MessageDigest::sha256(), &self.hmac_key)
            .map_err(WebauthnError::OpenSSLError)?;

        let mut seed = [0; 32];
        let buf = signer
            .sign_oneshot_to_vec(username)
            .map_err(WebauthnError::OpenSSLError)?;

        seed.copy_from_slice(&buf);

        // Seed the rng
        let mut seeded_rng = ChaCha8Rng::from_seed(seed);

        let credentials = D::generate(&mut seeded_rng);

        Ok(credentials)
    }
}

#[cfg(test)]
mod tests {
    use super::{FakePasskeyDistribution, WebauthnFakeCredentialGenerator};
    use crate::proto::Base64UrlSafeData;

    #[test]
    fn test_fake_credential_generator() {
        let _ = tracing_subscriber::fmt::try_init();

        let cred_gen: WebauthnFakeCredentialGenerator<FakePasskeyDistribution> =
            WebauthnFakeCredentialGenerator::new(&[0, 1, 2, 3]).unwrap();

        let cred_a = cred_gen.generate(b"a").unwrap();
        assert!(cred_a.is_empty());

        let cred_b = cred_gen.generate(b"b").unwrap();
        assert_eq!(
            cred_b,
            vec![Base64UrlSafeData::from(vec![
                77, 7, 210, 37, 212, 90, 185, 162, 81, 110, 242, 185, 204, 84, 84, 123, 155, 139,
                146, 230
            ])]
        );

        let cred_c = cred_gen.generate(b"c").unwrap();
        assert!(cred_c.is_empty());

        let cred_d = cred_gen.generate(b"d").unwrap();
        assert_eq!(
            cred_d,
            vec![Base64UrlSafeData::from(vec![
                203, 174, 48, 43, 223, 223, 211, 78, 99, 88, 240, 25, 90, 42, 86, 186, 239, 57,
                123, 81, 177, 173, 236, 214, 204, 222, 224, 134, 233, 143, 143, 144, 127, 23, 26,
                145, 217, 217, 110, 194, 235, 76, 2, 59, 56, 98, 47, 236, 103, 98, 235, 239, 195,
                140, 199, 239, 201, 11, 132, 227, 181, 7, 188, 240, 168
            ])]
        );

        let cred_e = cred_gen.generate(b"e").unwrap();
        assert_eq!(
            cred_e,
            vec![
                Base64UrlSafeData::from(vec![
                    207, 79, 70, 16, 136, 39, 65, 40, 104, 116, 214, 85, 66, 12, 175, 99, 203, 228,
                    60, 249, 118, 169, 28, 217, 161, 132, 3, 217, 119, 66, 235, 151, 138, 15, 26,
                    76, 161, 44, 225, 120, 34, 131, 48, 195, 116, 81, 178, 0, 218, 96, 167, 1, 70,
                    183, 20, 94, 115, 63, 12, 235, 189, 105, 104, 60, 77
                ]),
                Base64UrlSafeData::from(vec![
                    175, 118, 205, 177, 121, 39, 194, 157, 251, 53, 216, 180, 38, 22, 44, 155, 132,
                    155, 204, 68, 171, 98, 97, 114, 50, 58, 218, 238, 44, 154, 27, 140, 95, 90,
                    127, 210, 221, 177, 194, 44, 231, 178, 238, 239, 79, 222, 127, 164, 115, 65,
                    160, 6, 55, 150, 30, 140, 18, 159, 229, 159, 78, 216, 120, 27, 122
                ])
            ]
        );

        // Demonstrate that re-keying the generator yields different generated credential results.
        let alt_cred_gen: WebauthnFakeCredentialGenerator<FakePasskeyDistribution> =
            WebauthnFakeCredentialGenerator::new(&[3, 2, 1, 0]).unwrap();

        let alt_cred_a = alt_cred_gen.generate(b"a").unwrap();
        assert_ne!(alt_cred_a, cred_a);
        assert_eq!(
            alt_cred_a,
            vec![Base64UrlSafeData::from(vec![
                44, 141, 39, 252, 47, 212, 48, 123, 96, 131, 15, 213, 21, 149, 95, 147, 188, 152,
                201, 171, 245, 103, 22, 246, 211, 172, 143, 86, 97, 96, 109, 246, 23, 54, 13, 127,
                167, 107, 72, 235, 151, 144, 162, 200, 251, 93, 137, 8, 211, 197, 47, 115, 108,
                210, 62, 232, 246, 206, 36, 202, 94, 179, 254, 81, 59
            ])]
        );
    }
}
