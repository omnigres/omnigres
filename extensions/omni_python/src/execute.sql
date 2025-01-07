create function execute(code text) returns void
    language plpython3u
as
$$
    import os
    import importlib
    site_packages = os.path.expanduser(
        plpy.execute(plpy.prepare("""
                     select coalesce(
                        (select value from omni_python.config where name = 'site_packages'),
                        (select (setting || '/.omni_python/default') as value from pg_settings where name = 'data_directory'))
                        as value
       """))[0]['value'])
    import sys

    if sys.path[0] != site_packages:
        sys.path.insert(0, site_packages)
    importlib.invalidate_caches()
    del sys
    del os
    del importlib
    exec(compile(code, 'unnamed.py', 'exec'), globals(), locals())
$$;
