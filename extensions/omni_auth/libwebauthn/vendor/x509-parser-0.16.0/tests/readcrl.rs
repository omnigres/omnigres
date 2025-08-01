use x509_parser::prelude::*;

#[cfg(feature = "verify")]
#[test]
fn read_crl_verify() {
    const CA_DATA: &[u8] = include_bytes!("../assets/ca_minimalcrl.der");
    const CRL_DATA: &[u8] = include_bytes!("../assets/minimal.crl");

    let (_, x509_ca) = X509Certificate::from_der(CA_DATA).expect("could not parse certificate");
    let (_, crl) = parse_x509_crl(CRL_DATA).expect("could not parse revocation list");
    let res = crl.verify_signature(&x509_ca.tbs_certificate.subject_pki);
    eprintln!("Verification: {:?}", res);
    assert!(res.is_ok());
}

fn crl_idp<'a>(crl: &'a CertificateRevocationList) -> &'a IssuingDistributionPoint<'a> {
    let crl_idp = crl
        .tbs_cert_list
        .find_extension(&oid_registry::OID_X509_EXT_ISSUER_DISTRIBUTION_POINT)
        .expect("missing IDP extension");
    match crl_idp.parsed_extension() {
        ParsedExtension::IssuingDistributionPoint(crl_idp) => crl_idp,
        _ => panic!("wrong extension type"),
    }
}

#[test]
fn read_minimal_crl_idp() {
    const CRL_DATA: &[u8] = include_bytes!("../assets/crl-idp/minimal.der");
    let (_, crl) = parse_x509_crl(CRL_DATA).expect("could not parse revocation list");
    let crl_idp = crl_idp(&crl);

    let dp = crl_idp
        .distribution_point
        .as_ref()
        .expect("missing distribution point");
    let full_name = match dp {
        DistributionPointName::FullName(full_name) => full_name,
        DistributionPointName::NameRelativeToCRLIssuer(_) => {
            panic!("wrong distribution point name type")
        }
    };
    assert_eq!(full_name.len(), 1);
    let uri = match full_name.first().unwrap() {
        GeneralName::URI(uri) => *uri,
        _ => panic!("wrong general name type"),
    };
    assert_eq!(uri, "http://crl.trustcor.ca/sub/dv-ssl-rsa-s-0.crl");

    assert!(!crl_idp.only_contains_user_certs);
    assert!(!crl_idp.only_contains_ca_certs);
    assert!(crl_idp.only_some_reasons.is_none());
    assert!(!crl_idp.only_contains_attribute_certs);
}

#[test]
fn test_only_user_crl_idp() {
    const CRL_DATA: &[u8] = include_bytes!("../assets/crl-idp/only_user_certs.der");
    let (_, crl) = parse_x509_crl(CRL_DATA).expect("could not parse revocation list");
    let crl_idp = crl_idp(&crl);

    assert!(crl_idp.only_contains_user_certs);
    assert!(!crl_idp.only_contains_ca_certs);
    assert!(crl_idp.only_some_reasons.is_none());
    assert!(!crl_idp.only_contains_attribute_certs);
}

#[test]
fn test_only_ca_crl_idp() {
    const CRL_DATA: &[u8] = include_bytes!("../assets/crl-idp/only_ca_certs.der");
    let (_, crl) = parse_x509_crl(CRL_DATA).expect("could not parse revocation list");
    let crl_idp = crl_idp(&crl);

    assert!(!crl_idp.only_contains_user_certs);
    assert!(crl_idp.only_contains_ca_certs);
    assert!(crl_idp.only_some_reasons.is_none());
    assert!(!crl_idp.only_contains_attribute_certs);
}

#[test]
fn test_only_some_reasons_crl_idp() {
    const CRL_DATA: &[u8] = include_bytes!("../assets/crl-idp/only_some_reasons.der");
    let (_, crl) = parse_x509_crl(CRL_DATA).expect("could not parse revocation list");
    let crl_idp = crl_idp(&crl);

    assert!(!crl_idp.only_contains_user_certs);
    assert!(!crl_idp.only_contains_ca_certs);
    assert!(!crl_idp.only_contains_attribute_certs);

    let reasons = crl_idp
        .only_some_reasons
        .as_ref()
        .expect("missing only_some_reasons");
    assert!(reasons.key_compromise());
    assert!(reasons.affilation_changed());
}

#[test]
fn test_only_attribute_cers_crl_idp() {
    const CRL_DATA: &[u8] = include_bytes!("../assets/crl-idp/only_attribute_certs.der");
    let (_, crl) = parse_x509_crl(CRL_DATA).expect("could not parse revocation list");
    let crl_idp = crl_idp(&crl);

    assert!(!crl_idp.only_contains_user_certs);
    assert!(!crl_idp.only_contains_ca_certs);
    assert!(crl_idp.only_some_reasons.is_none());
    assert!(crl_idp.only_contains_attribute_certs);
}
