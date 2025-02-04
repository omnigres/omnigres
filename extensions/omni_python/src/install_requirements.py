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

requirements_txt = tempfile.mktemp()
with open(requirements_txt, 'w') as f:
    f.write(requirements)

class PipOutputHandler:
    def __init__(self):
        pass

    def write(selfself, text):
        plpy.notice(text.strip('\n'))

    def flush(self):
        pass

os.makedirs(site_packages, exist_ok=True)
stderr_str = io.StringIO()
stdout_handler = PipOutputHandler()
with contextlib.redirect_stdout(stdout_handler), contextlib.redirect_stderr(stderr_str):
    try:
        from pip._internal.cli.main import main as pip

        rc = pip(["install", "--disable-pip-version-check", "--upgrade", "-r", requirements_txt, "--target", site_packages]
                 + (["--extra-index-url", index] if index is not None else [])
                 + [item for x in find_links for item in ("--find-links", x)]
                 )
        if rc != 0:
            raise SystemExit(rc)
    except SystemExit as e:
        plpy.error("pip failure", detail=stderr_str.getvalue())
