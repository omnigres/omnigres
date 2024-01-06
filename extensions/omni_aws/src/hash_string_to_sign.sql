create function hash_string_to_sign(
    algorithm text,
    ts8601 timestamp with time zone,
    region text,
    service text,
    canonical_request_hash text,
    secret_access_key text)
    returns text
    language sql
    immutable
as
$$
select
    encode(hmac((((((((algorithm || chr(10) ||
                       to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD"T"HH24MISS"Z"') || chr(10) ||
                       to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD')) || '/' || region) || '/') ||
                    service) || '/aws4_request') || chr(10)) || canonical_request_hash)::bytea, hmac('aws4_request',
                                                                                                     hmac(
                                                                                                             service::bytea,
                                                                                                             hmac(
                                                                                                                     region::bytea,
                                                                                                                     hmac(
                                                                                                                             to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD'),
                                                                                                                             ('AWS4' || secret_access_key),
                                                                                                                             'sha256'),
                                                                                                                     'sha256'),
                                                                                                             'sha256'),
                                                                                                     'sha256'),
                'sha256'), 'hex')
$$;