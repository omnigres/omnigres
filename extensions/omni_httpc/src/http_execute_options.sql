create function http_execute_options(http2_ratio integer default 0, http3_ratio integer default 0,
                                     force_cleartext_http2 bool default false,
                                     timeout int default null,
                                     first_byte_timeout int default null,
                                     follow_redirects bool default true,
                                     allow_self_signed_cert bool default false,
                                     cacerts text[] default null) returns http_execute_options
as
$$
select row (http2_ratio, http3_ratio, force_cleartext_http2, timeout, first_byte_timeout, follow_redirects, allow_self_signed_cert, cacerts)::omni_httpc.valid_http_execute_options
$$ language sql immutable;