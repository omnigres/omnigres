create function aws_v4_headers(
    method text,
    uri omni_web.uri,
    query text,
    payload bytea,
    access_key_id text,
    secret_access_key text,
    region text,
    service text,
    ts8601 timestamp with time zone,
    additional_headers omni_http.http_headers default array []::omni_http.http_headers)
    returns omni_http.http_headers
    language plpgsql
    immutable
as
$$
declare
    all_headers          omni_http.http_headers;
    payload_hash         text := encode(digest(payload, 'sha256'), 'hex');
    amz_date             text := to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD"T"HH24MISS"Z"');
    date_text            text := to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD');
    canonical_headers    text[];
    signed_headers_names text[];
    authorization        text;
    host                 text := uri.host || coalesce(':' || uri.port, '');
    canonical_path       text;
begin
    -- 1. Prepare mandatory headers
    all_headers := array [
        omni_http.http_header('host', host),
        omni_http.http_header('x-amz-content-sha256', payload_hash),
        omni_http.http_header('x-amz-date', amz_date)
        ];

    -- 2. Add additional headers
    all_headers := all_headers || additional_headers;

    -- 3. Prepare canonical headers and signed headers names
    -- AWS requires header names to be lowercased and sorted.
    -- Header values should have leading/trailing whitespace removed and multiple spaces collapsed.
    select
        array_agg(lower(name) || ':' || trim(regexp_replace(value, '\s+', ' ', 'g')) order by lower(name)),
        array_agg(lower(name) order by lower(name))
    into
        canonical_headers,
        signed_headers_names
    from
        unnest(all_headers);

    -- 4. Canonical path
    canonical_path := coalesce(uri.path, '/');
    if not canonical_path like '/%' then
        canonical_path := '/' || canonical_path;
    end if;

    -- 5. Generate Authorization header
    authorization := 'AWS4-HMAC-SHA256 Credential=' || access_key_id || '/' ||
                     date_text || '/' || region || '/' || service || '/aws4_request, ' ||
                     'SignedHeaders=' || array_to_string(signed_headers_names, ';') || ', ' ||
                     'Signature=' || omni_aws.hash_string_to_sign(
                             'AWS4-HMAC-SHA256',
                             ts8601,
                             region,
                             service,
                             omni_aws.hash_canonical_request(
                                     method,
                                     canonical_path,
                                     query,
                                     canonical_headers,
                                     signed_headers_names,
                                     payload_hash
                                 ),
                             secret_access_key
                         );

    return all_headers || omni_http.http_header('Authorization', authorization);
end;
$$;
