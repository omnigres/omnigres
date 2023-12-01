CREATE FUNCTION hash_string_to_sign(
    algorithm TEXT,
    ts8601 TIMESTAMP WITH TIME ZONE,
    region TEXT,
    service TEXT,
    canonical_request_hash TEXT,
    secret_access_key TEXT)
RETURNS TEXT
LANGUAGE plpgsql
AS $$
DECLARE
    intermediate_key TEXT;
BEGIN
    intermediate_key := hmac('aws4_request',
                            hmac(service::BYTEA,
                                 hmac(region::BYTEA,
                                      hmac(to_char((ts8601 AT TIME ZONE 'UTC'), 'YYYYMMDD'),
                                           ('AWS4' || secret_access_key),
                                           'sha256'),
                                      'sha256'),
                                 'sha256'),
                            'sha256')::TEXT;

    RETURN encode(hmac((algorithm || CHR(10) ||
                       to_char((ts8601 AT TIME ZONE 'UTC'), 'YYYYMMDD"T"HH24MISS"Z"') || CHR(10) ||
                       to_char((ts8601 AT TIME ZONE 'UTC'), 'YYYYMMDD') || '/' || region || '/' ||
                       service || '/aws4_request' || CHR(10) || canonical_request_hash)::BYTEA,
                      intermediate_key::BYTEA,
                      'sha256'), 'hex');
END;
$$;
