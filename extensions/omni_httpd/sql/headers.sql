select omni_httpd.http_header_get(array [omni_httpd.http_header('Host', 'omnihost')], 'Host');
select omni_httpd.http_header_get(array [omni_httpd.http_header('Host', 'omnihost')], 'host');
select
    omni_httpd.http_header_get_all(
            array [omni_httpd.http_header('Accept', 'application/xml'), omni_httpd.http_header('Accept', 'application/json')],
            'accept');
