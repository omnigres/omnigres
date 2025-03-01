import io
import contextlib
import os
import tempfile

site_packages = os.path.expanduser(
    plpy.execute(plpy.prepare("""
                     select coalesce(
                        (select value from omni_python.config where name = 'site_packages'),
                        (select (setting || '/.omni_python/default') as value from pg_settings where name = 'data_directory'))
                        as value
       """))[0]['value'])

if not os.path.exists(site_packages):
    os.makedirs(site_packages)

index = plpy.execute(
    plpy.prepare(
        "select (select value from omni_python.config where name = 'extra_pip_index_url') as value"))[0][
    'value']

find_links = plpy.execute(
    plpy.prepare(
        "select array_agg(value) as value from (select value from omni_python.config where name = 'pip_find_links' union all select * from omni_python.wheel_paths()) t"))[
                 0][
                 'value'] or []

os.makedirs(site_packages, exist_ok=True)
stderr_str = io.StringIO()
stdout_str = io.StringIO()
with contextlib.redirect_stdout(stdout_str), contextlib.redirect_stderr(stderr_str):
    try:
        from pip._internal.cli.main import main as pip

        rc = pip(["freeze", "--path", site_packages])
        if rc != 0:
            raise SystemExit(rc)
    except SystemExit as e:
        plpy.error("pip failure", detail=stderr_str.getvalue())

result = [
    {"name": name, "version": version}
    for line in stdout_str.getvalue().strip().split("\n")
    for name, version in [line.split("==")]
]

return result