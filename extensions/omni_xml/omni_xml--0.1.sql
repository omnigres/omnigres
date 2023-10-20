create function xpath(document text, query text)
    returns table
            (
                path text,
                value text
            )
    language c
as
'MODULE_PATHNAME';