create procedure postgrest_cors(request omni_httpd.http_request, response inout omni_httpd.http_outcome)
    language plpgsql as
$$
declare
    origin text;
begin
    if request.method = 'OPTIONS' then
        origin := omni_http.http_header_get(request.headers, 'origin');
        response := omni_httpd.http_response(null::text,
                                             headers => array [omni_http.http_header('Access-Control-Allow-Origin', origin),
                                                 omni_http.http_header('Access-Control-Allow-Credentials', 'true'),
                                                 omni_http.http_header('Access-Control-Allow-Methods',
                                                                       'GET, POST, PATCH, PUT, DELETE, OPTIONS, HEAD'),
                                                 omni_http.http_header('Access-Control-Allow-Headers',
                                                                       'Authorization, Content-Type, Accept, Accept-Language, Content-Language'),
                                                 omni_http.http_header('Access-Control-Max-Age', '86400')
                                                 ]);
    end if;
end;
$$;