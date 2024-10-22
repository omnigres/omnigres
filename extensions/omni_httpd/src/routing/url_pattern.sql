CREATE OR REPLACE FUNCTION url_pattern_to_regex(
    pattern text
) RETURNS text AS $$
DECLARE
    result text;
BEGIN
    -- Basic conversion of URL pattern to regex
    result := pattern;
    
    -- Replace :param with capturing groups
    result := regexp_replace(result, ':(\w+)', '([^/]+)', 'g');
    
    -- Escape special regex characters
    result := regexp_replace(result, '\.', '\.', 'g');
    
    -- Add start and end anchors
    result := '^' || result || '$';
    
    RETURN result;
END;
$$ LANGUAGE plpgsql;