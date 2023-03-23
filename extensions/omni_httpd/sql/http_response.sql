-- NULL status
select omni_httpd.http_response(status => null, body => 'test');

-- NULL headers
select omni_httpd.http_response(headers => null, body => 'test');

-- NULL body
select omni_httpd.http_response(body => null);
select omni_httpd.http_response();

-- Text body
select omni_httpd.http_response(body => 'text');

-- JSON body
select omni_httpd.http_response(body => '{}'::json);

-- JSONB body
select omni_httpd.http_response(body => '{}'::jsonb);

-- Binary body
select omni_httpd.http_response(body => convert_to('binary', 'UTF8'));

-- Specifying status
select omni_httpd.http_response(status => 404);

-- Specifying headers
select
    omni_httpd.http_response(headers => array [omni_httpd.http_header('test', 'value')]::omni_httpd.http_header[],
                             body => null);

-- Merging headers with inferred ones
select
    omni_httpd.http_response(headers => array [omni_httpd.http_header('test', 'value')]::omni_httpd.http_header[],
                             body => 'test');

-- Overriding content type
select
    omni_httpd.http_response(headers => array [omni_httpd.http_header('content-type', 'text/html')], body => 'test');
-- Overriding content type, with a different case
select
    omni_httpd.http_response(headers => array [omni_httpd.http_header('Content-Type', 'text/html')], body => 'test');

--- Shortcut syntax with body first
select omni_httpd.http_response('test');
select omni_httpd.http_response('"test"'::json);
select omni_httpd.http_response('"test"'::jsonb);
