create function functions(code text, filename text default null, fs text default null, fs_type regtype default null)
    returns table
            (
                name     name,
                argnames name[],
                argtypes regtype[],
                rettype  regtype,
                code     text
            )
    language plpython3u
as
$$
/*{% include "../src/functions.py" %}*/
$$;
