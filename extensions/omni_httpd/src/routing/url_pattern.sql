create or replace function url_pattern_to_regex(
    pattern text
) returns text as $$
declare
    result text;
begin
    -- Basic conversion of URL pattern to regex
    result := pattern;
    
    -- Replace :param with capturing groups
    result := regexp_replace(result, ':(\w+)', '([^/]+)', 'g');
    
    -- Escape special regex characters
    result := regexp_replace(result, '\.', '\.', 'g');
    
    -- Add start and end anchors
    result := '^' || result || '$';
    
    return result;
end;
$$ language plpgsql;