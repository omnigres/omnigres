use base64urlsafedata::Base64UrlSafeData;
use openssl::error::ErrorStack as OpenSSLErrorStack;
use openssl::{hash, x509};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

use uuid::Uuid;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct DeviceDescription {
    pub(crate) en: String,
    pub(crate) localised: BTreeMap<String, String>,
}

impl DeviceDescription {
    /// A default description of device.
    pub fn description_en(&self) -> &str {
        self.en.as_str()
    }

    /// A map of locale identifiers to a localised description of the device.
    /// If the request locale is not found, you should try other user preferenced locales
    /// falling back to the default value.
    pub fn description_localised(&self) -> &BTreeMap<String, String> {
        &self.localised
    }
}

/// A serialised Attestation CA.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SerialisableAttestationCa {
    pub(crate) ca: Base64UrlSafeData,
    pub(crate) aaguids: BTreeMap<Uuid, DeviceDescription>,
    pub(crate) blanket_allow: bool,
}

/// A structure representing an Attestation CA and other options associated to this CA.
///
/// Generally depending on the Attestation CA in use, this can help determine properties
/// of the authenticator that is in use.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(
    try_from = "SerialisableAttestationCa",
    into = "SerialisableAttestationCa"
)]
pub struct AttestationCa {
    /// The x509 root CA of the attestation chain that a security key will be attested to.
    ca: x509::X509,
    /// If not empty, the set of acceptable AAGUIDS (Device Ids) that are allowed to be
    /// attested as trusted by this CA. AAGUIDS that are not in this set, but signed by
    /// this CA will NOT be trusted.
    aaguids: BTreeMap<Uuid, DeviceDescription>,
    blanket_allow: bool,
}

#[allow(clippy::from_over_into)]
impl Into<SerialisableAttestationCa> for AttestationCa {
    fn into(self) -> SerialisableAttestationCa {
        SerialisableAttestationCa {
            ca: Base64UrlSafeData::from(self.ca.to_der().expect("Invalid DER")),
            aaguids: self.aaguids,
            blanket_allow: self.blanket_allow,
        }
    }
}

impl TryFrom<SerialisableAttestationCa> for AttestationCa {
    type Error = OpenSSLErrorStack;

    fn try_from(data: SerialisableAttestationCa) -> Result<Self, Self::Error> {
        Ok(AttestationCa {
            ca: x509::X509::from_der(data.ca.as_slice())?,
            aaguids: data.aaguids,
            blanket_allow: data.blanket_allow,
        })
    }
}

impl AttestationCa {
    pub fn ca(&self) -> &x509::X509 {
        &self.ca
    }

    pub fn aaguids(&self) -> &BTreeMap<Uuid, DeviceDescription> {
        &self.aaguids
    }

    pub fn blanket_allow(&self) -> bool {
        self.blanket_allow
    }

    /// Retrieve the Key Identifier for this Attestation Ca
    pub fn get_kid(&self) -> Result<Vec<u8>, OpenSSLErrorStack> {
        self.ca
            .digest(hash::MessageDigest::sha256())
            .map(|bytes| bytes.to_vec())
    }

    fn insert_device(
        &mut self,
        aaguid: Uuid,
        desc_english: String,
        desc_localised: BTreeMap<String, String>,
    ) {
        self.blanket_allow = false;
        self.aaguids.insert(
            aaguid,
            DeviceDescription {
                en: desc_english,
                localised: desc_localised,
            },
        );
    }

    fn new_from_pem(data: &[u8]) -> Result<Self, OpenSSLErrorStack> {
        Ok(AttestationCa {
            ca: x509::X509::from_pem(data)?,
            aaguids: BTreeMap::default(),
            blanket_allow: true,
        })
    }

    fn union(&mut self, other: &Self) {
        // if either is a blanket allow, we just do that.
        if self.blanket_allow || other.blanket_allow {
            self.blanket_allow = true;
            self.aaguids.clear();
        } else {
            self.blanket_allow = false;
            for (o_aaguid, o_device) in other.aaguids.iter() {
                // We can use the entry api here since o_aaguid is copy.
                self.aaguids
                    .entry(*o_aaguid)
                    .or_insert_with(|| o_device.clone());
            }
        }
    }

    fn intersection(&mut self, other: &Self) {
        // If they are a blanket allow, do nothing, we are already
        // more restrictive, or we also are a blanket allow
        if other.blanket_allow() {
            // Do nothing
        } else if self.blanket_allow {
            // Just set our aaguids to other, and remove our blanket allow.
            self.blanket_allow = false;
            self.aaguids = other.aaguids.clone();
        } else {
            // Only keep what is also in other.
            self.aaguids
                .retain(|s_aaguid, _| other.aaguids.contains_key(s_aaguid))
        }
    }

    fn can_retain(&self) -> bool {
        // Only retain a CA if it's a blanket allow, or has aaguids remaining.
        self.blanket_allow || !self.aaguids.is_empty()
    }
}

/// A list of AttestationCas and associated options.
#[derive(Debug, Default, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct AttestationCaList {
    /// The set of CA's that we trust in this Operation
    cas: BTreeMap<Base64UrlSafeData, AttestationCa>,
}

impl TryFrom<&[u8]> for AttestationCaList {
    type Error = OpenSSLErrorStack;

    fn try_from(data: &[u8]) -> Result<Self, Self::Error> {
        let mut new = Self::default();
        let att_ca = AttestationCa::new_from_pem(data)?;
        new.insert(att_ca)?;
        Ok(new)
    }
}

impl AttestationCaList {
    pub fn cas(&self) -> &BTreeMap<Base64UrlSafeData, AttestationCa> {
        &self.cas
    }

    pub fn clear(&mut self) {
        self.cas.clear()
    }

    pub fn len(&self) -> usize {
        self.cas.len()
    }

    /// Determine if this attestation list contains any members.
    pub fn is_empty(&self) -> bool {
        self.cas.is_empty()
    }

    /// Insert a new att_ca into this Attestation Ca List
    pub fn insert(
        &mut self,
        att_ca: AttestationCa,
    ) -> Result<Option<AttestationCa>, OpenSSLErrorStack> {
        // Get the key id (kid, digest).
        let att_ca_dgst = att_ca.get_kid()?;
        Ok(self.cas.insert(att_ca_dgst.into(), att_ca))
    }

    /// Join two CA lists into one, taking all elements from both.
    pub fn union(&mut self, other: &Self) {
        for (o_kid, o_att_ca) in other.cas.iter() {
            if let Some(s_att_ca) = self.cas.get_mut(o_kid) {
                s_att_ca.union(o_att_ca)
            } else {
                self.cas.insert(o_kid.clone(), o_att_ca.clone());
            }
        }
    }

    /// Retain only the CA's and devices that exist in self and other.
    pub fn intersection(&mut self, other: &Self) {
        self.cas.retain(|s_kid, s_att_ca| {
            // First, does this exist in our partner?
            if let Some(o_att_ca) = other.cas.get(s_kid) {
                // Now, intersect.
                s_att_ca.intersection(o_att_ca);
                if s_att_ca.can_retain() {
                    // Still as elements, retain.
                    true
                } else {
                    // Nothing remains, remove.
                    false
                }
            } else {
                // Not in other, remove.
                false
            }
        })
    }
}

#[derive(Default)]
pub struct AttestationCaListBuilder {
    cas: BTreeMap<Vec<u8>, AttestationCa>,
}

impl AttestationCaListBuilder {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn insert_device_x509(
        &mut self,
        ca: x509::X509,
        aaguid: Uuid,
        desc_english: String,
        desc_localised: BTreeMap<String, String>,
    ) -> Result<(), OpenSSLErrorStack> {
        let kid = ca
            .digest(hash::MessageDigest::sha256())
            .map(|bytes| bytes.to_vec())?;

        let mut att_ca = if let Some(att_ca) = self.cas.remove(&kid) {
            att_ca
        } else {
            AttestationCa {
                ca,
                aaguids: BTreeMap::default(),
                blanket_allow: false,
            }
        };

        att_ca.insert_device(aaguid, desc_english, desc_localised);

        self.cas.insert(kid, att_ca);

        Ok(())
    }

    pub fn insert_device_der(
        &mut self,
        ca_der: &[u8],
        aaguid: Uuid,
        desc_english: String,
        desc_localised: BTreeMap<String, String>,
    ) -> Result<(), OpenSSLErrorStack> {
        let ca = x509::X509::from_der(ca_der)?;
        self.insert_device_x509(ca, aaguid, desc_english, desc_localised)
    }

    pub fn insert_device_pem(
        &mut self,
        ca_pem: &[u8],
        aaguid: Uuid,
        desc_english: String,
        desc_localised: BTreeMap<String, String>,
    ) -> Result<(), OpenSSLErrorStack> {
        let ca = x509::X509::from_pem(ca_pem)?;
        self.insert_device_x509(ca, aaguid, desc_english, desc_localised)
    }

    pub fn build(self) -> AttestationCaList {
        let cas = self
            .cas
            .into_iter()
            .map(|(kid, att_ca)| (kid.into(), att_ca))
            .collect();

        AttestationCaList { cas }
    }
}
