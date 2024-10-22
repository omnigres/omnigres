create function handler(postgrest_cors_route) returns omni_httpd.http_outcome
    strict
    language plpgsql as
$$
declare

begin
    return omni_httpd.http_response(null::text,
                                    headers => array [omni_http.http_header('Access-Control-Allow-Origin', $1.origin),
                                        omni_http.http_header('Access-Control-Allow-Credentials', 'true'),
                                        omni_http.http_header('Access-Control-Allow-Methods',
                                                              'GET, POST, PATCH, PUT, DELETE, OPTIONS, HEAD'),
                                        omni_http.http_header('Access-Control-Allow-Headers',
                                                              'Authorization, Content-Type, Accept, Accept-Language, Content-Language'),
                                        omni_http.http_header('Access-Control-Max-Age', '86400')
                                        ]);
end;
$$;