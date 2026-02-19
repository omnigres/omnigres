-- Router types and functions using omni_regex

-- Route pattern type that uses omni_regex for pattern matching
create type route_pattern as
(
    pattern     text,
    compiled_re omni_regex.regex
);

-- Route definition type
create type route as
(
    pattern route_pattern,
    method  omni_http.http_method
);

-- Route match result type
create type route_match as
(
    captures jsonb,
    route    route
);

-- Function to create a route pattern
create function route_pattern(pattern text)
    returns route_pattern
as
$$
declare
    result route_pattern;
begin
    result.pattern := pattern;
    result.compiled_re := omni_regex.compile(pattern);
    return result;
end;
$$ language plpgsql immutable;

-- Function to create a route
create function route(pattern text, method omni_http.http_method)
    returns route
as
$$
select row (route_pattern(pattern), method)::route;
$$ language sql immutable;

-- Function to match route pattern against path
create function match(pattern route_pattern, path text)
    returns jsonb
as
$$
select omni_regex.match(pattern.compiled_re, path);
$$ language sql immutable;

-- Function to match HTTP request against route
create function match(r route, req http_request)
    returns route_match
as
$$
declare
    captures jsonb;
begin
    if req.method != r.method then
        return null;
    end if;

    captures := match(r.pattern, req.path);
    if captures is null then
        return null;
    end if;

    return row (captures, r)::route_match;
end;
$$ language plpgsql immutable;

/*{% include "../src/router.sql" %}*/

-- Add helpful comments
comment on type route_pattern is 'Route pattern with compiled regex';
comment on type route is 'HTTP route definition with pattern and method';
comment on type route_match is 'Result of matching a route against a request';