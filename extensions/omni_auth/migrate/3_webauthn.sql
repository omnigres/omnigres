create function webauthn_start_passkey_registration(rp_id text, rp_origin text, uuid text, user_name text,
                                                    user_display_name text) returns text
    language c as
'MODULE_PATHNAME',
'webauthn_start_passkey_registration_';
