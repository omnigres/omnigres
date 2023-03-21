-- NULL status
SELECT omni_httpd.http_response(status => null, body => 'test');

-- NULL headers
SELECT omni_httpd.http_response(headers => null, body => 'test');

-- NULL body
SELECT omni_httpd.http_response(body => null);

-- Text body
SELECT omni_httpd.http_response(body => 'text');

-- JSON body
SELECT omni_httpd.http_response(body => '{}'::json);

-- JSONB body
SELECT omni_httpd.http_response(body => '{}'::jsonb);

-- Binary body
SELECT omni_httpd.http_response(body => convert_to('binary', 'UTF8'));

-- Specifying status
SELECT omni_httpd.http_response(status => 404);

-- Specifying headers
SELECT omni_httpd.http_response(headers => array[omni_httpd.http_header('test', 'value')]::omni_httpd.http_header[], body => null);

-- Merging headers with inferred ones
SELECT omni_httpd.http_response(headers => array[omni_httpd.http_header('test', 'value')]::omni_httpd.http_header[], body => 'test');

-- Overriding content type
SELECT omni_httpd.http_response(headers => array[omni_httpd.http_header('content-type', 'text/html')], body => 'test');
-- Overriding content type, with a different case
SELECT omni_httpd.http_response(headers => array[omni_httpd.http_header('Content-Type', 'text/html')], body => 'test');
