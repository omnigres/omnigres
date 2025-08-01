#[allow(clippy::unusual_byte_groupings)]
pub fn get(openssl_version: Option<u64>, libressl_version: Option<u64>) -> Vec<&'static str> {
    let mut cfgs = vec![];

    if let Some(libressl_version) = libressl_version {
        cfgs.push("libressl");

        if libressl_version >= 0x2_05_01_00_0 {
            cfgs.push("libressl251");
        }
        if libressl_version >= 0x2_05_02_00_0 {
            cfgs.push("libressl252");
        }
        if libressl_version >= 0x2_06_01_00_0 {
            cfgs.push("libressl261");
        }
        if libressl_version >= 0x2_07_00_00_0 {
            cfgs.push("libressl270");
        }
        if libressl_version >= 0x2_07_01_00_0 {
            cfgs.push("libressl271");
        }
        if libressl_version >= 0x2_07_03_00_0 {
            cfgs.push("libressl273");
        }
        if libressl_version >= 0x2_08_00_00_0 {
            cfgs.push("libressl280");
        }
        if libressl_version >= 0x2_08_01_00_0 {
            cfgs.push("libressl281");
        }
        if libressl_version >= 0x2_09_01_00_0 {
            cfgs.push("libressl291");
        }
        if libressl_version >= 0x3_01_00_00_0 {
            cfgs.push("libressl310");
        }
        if libressl_version >= 0x3_02_01_00_0 {
            cfgs.push("libressl321");
        }
        if libressl_version >= 0x3_03_02_00_0 {
            cfgs.push("libressl332");
        }
        if libressl_version >= 0x3_04_00_00_0 {
            cfgs.push("libressl340");
        }
        if libressl_version >= 0x3_05_00_00_0 {
            cfgs.push("libressl350");
        }
        if libressl_version >= 0x3_06_00_00_0 {
            cfgs.push("libressl360");
        }
        if libressl_version >= 0x3_07_00_00_0 {
            cfgs.push("libressl370");
        }
        if libressl_version >= 0x3_08_00_00_0 {
            cfgs.push("libressl380");
        }
        if libressl_version >= 0x3_08_01_00_0 {
            cfgs.push("libressl381");
        }
        if libressl_version >= 0x3_08_02_00_0 {
            cfgs.push("libressl382");
        }
        if libressl_version >= 0x3_09_00_00_0 {
            cfgs.push("libressl390");
        }
        if libressl_version >= 0x4_00_00_00_0 {
            cfgs.push("libressl400");
        }
        if libressl_version >= 0x4_01_00_00_0 {
            cfgs.push("libressl410");
        }
    } else {
        let openssl_version = openssl_version.unwrap();

        if openssl_version >= 0x3_04_00_00_0 {
            cfgs.push("ossl340");
        }
        if openssl_version >= 0x3_03_00_00_0 {
            cfgs.push("ossl330");
        }
        if openssl_version >= 0x3_02_00_00_0 {
            cfgs.push("ossl320");
        }
        if openssl_version >= 0x3_00_00_00_0 {
            cfgs.push("ossl300");
        }
        if openssl_version >= 0x1_00_01_00_0 {
            cfgs.push("ossl101");
        }
        if openssl_version >= 0x1_00_02_00_0 {
            cfgs.push("ossl102");
        }
        if openssl_version >= 0x1_00_02_06_0 {
            cfgs.push("ossl102f");
        }
        if openssl_version >= 0x1_00_02_08_0 {
            cfgs.push("ossl102h");
        }
        if openssl_version >= 0x1_01_00_00_0 {
            cfgs.push("ossl110");
        }
        if openssl_version >= 0x1_01_00_06_0 {
            cfgs.push("ossl110f");
        }
        if openssl_version >= 0x1_01_00_07_0 {
            cfgs.push("ossl110g");
        }
        if openssl_version >= 0x1_01_00_08_0 {
            cfgs.push("ossl110h");
        }
        if openssl_version >= 0x1_01_01_00_0 {
            cfgs.push("ossl111");
        }
        if openssl_version >= 0x1_01_01_02_0 {
            cfgs.push("ossl111b");
        }
        if openssl_version >= 0x1_01_01_03_0 {
            cfgs.push("ossl111c");
        }
        if openssl_version >= 0x1_01_01_04_0 {
            cfgs.push("ossl111d");
        }
    }

    cfgs
}
