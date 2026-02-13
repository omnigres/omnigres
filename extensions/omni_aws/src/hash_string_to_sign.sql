create function hash_string_to_sign(
    algorithm text,
    ts8601 timestamp with time zone,
    region text,
    service text,
    canonical_request_hash text,
    secret_access_key text)
    returns text
    language plpgsql
    immutable
as
$$
declare
    date_text      text  := to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD');
    k_date         bytea := hmac(date_text, ('AWS4' || secret_access_key), 'sha256');
    k_region       bytea := hmac(region::bytea, k_date, 'sha256');
    k_service      bytea := hmac(service::bytea, k_region, 'sha256');
    k_signing      bytea := hmac('aws4_request', k_service, 'sha256');
    string_to_sign text;
begin
    string_to_sign := algorithm || chr(10) ||
                      to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD"T"HH24MISS"Z"') || chr(10) ||
                      date_text || '/' || region || '/' || service || '/aws4_request' || chr(10) ||
                      canonical_request_hash;

    return encode(hmac(string_to_sign::bytea, k_signing, 'sha256'), 'hex');
end;
$$;
