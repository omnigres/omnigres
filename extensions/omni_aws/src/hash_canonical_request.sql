create function hash_canonical_request(method text,
                                       uri text,
                                       query text,
                                       headers text[],
                                       signed_headers text[],
                                       payload_hash text)
    returns text
    language sql
    immutable
as
$$
select
    encode(digest(omni_aws.canonical_request(method, uri, query, headers, signed_headers, payload_hash), 'sha256'),
           'hex')
$$;