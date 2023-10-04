create function functions(code text, filename text default null)
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
    import os
    import ast
    import decimal
    import typing
    import types
    import sys
    # types.UnionType has only been available since Python 3.10
    if sys.version_info >= (3, 10):
        from types import UnionType
    else:
        from typing import Union as UnionType

    site_packages = os.path.expanduser(
        plpy.execute(plpy.prepare("select current_setting('omni_python.site_packages', true)"))[0][
            'current_setting'] or "~/.omni_python/default")

    import sys
    sys.path.insert(0, site_packages)
    del sys
    import omni_python

    code_locals = {}
    code_globals = globals()

    try:
        exec(compile(code, filename or 'unnamed.py', 'exec'), code_globals, code_locals)
    except SyntaxError:
        pass

    pg_functions = []
    for name, value in code_locals.items():
        if callable(value):
            if hasattr(value, '__pg_stored_procedure__'):
                pg_functions.append((name, value))

    __types__ = {str: 'text', bool: 'boolean', bytes: 'bytea', int: 'int',
                 decimal.Decimal: 'numeric', float: 'double precision'}

    def resolve_type(function, arg):
        type = function.__annotations__[arg]
        if hasattr(type, '__pg_type_hint__') and callable(type.__pg_type_hint__):
            type.__pg_type_hint__.__globals__.update(code_locals)
            type = type.__pg_type_hint__()
        if type is None:
            return 'unknown'
        else:
            # list
            if isinstance(type, types.GenericAlias) and type.__origin__ == list:
                if len(type.__args__) > 1:
                    raise TypeError("lists can only be of one parameter")
                type = type.__args__[0]
                return '{type}[]'.format(type=__types__.get(type, 'unknown'))
            # Optional
            if type.__class__ in (typing.Optional[bool].__class__, UnionType) and len(
                    type.__args__) == 2 and None.__class__ in type.__args__:
                type = [t for t in type.__args__ if t != None.__class__][0]
            # Custom type
            if type.__class__ == typing.Annotated[int, 0].__class__ and isinstance(type.__metadata__[0],
                                                                                   omni_python.pgtype):
                return type.__metadata__[0].name
            try:
                return __types__.get(type, 'unknown')
            except TypeError:
                return 'unknown'

    def process_argument(function, arg):
        import ast
        type = function.__annotations__[arg]
        if hasattr(type, '__pg_type_hint__') and callable(type.__pg_type_hint__):
            type.__pg_type_hint__.__globals__.update(code_locals)
            type = type.__pg_type_hint__()
        if (type.__class__ == typing.Annotated[int, 0].__class__ and isinstance(type.__metadata__[0],
                                                                                omni_python.pgtype) and
                type.__metadata__[1] == "composite"):
            klass = type.__args__[0]
            if klass.__module__ == '__main__':
                return f"{klass.__name__}(**{arg})"
            else:
                lookup = ast.unparse(
                    ast.Subscript(value=ast.Name(id='sys.modules', ctx=ast.Load()),
                                  slice=ast.Constant(value=klass.__module__),
                                  ctx=ast.Load()))
                return f"{lookup}.{klass.__name__}(**{arg})"
        else:
            return arg

    site_packages = plpy.quote_literal(
        os.path.expanduser(plpy.execute(plpy.prepare("select current_setting('omni_python.site_packages', true)"))[0][
                               'current_setting'] or "~/.omni_python/default"))

    preamble = f"import sys ; sys.path.insert(0, {site_packages})"

    import inspect

    return [(name,
             [a for a in inspect.getfullargspec(f).args],
             [resolve_type(f, a) for a in inspect.getfullargspec(f).args], resolve_type(f, 'return'),
             "{preamble}\n{code}\nreturn {name}({args})".format(preamble=preamble, code=code,
                                                                name=name,
                                                                args=', '.join(
                                                                    [process_argument(f, a) for a in
                                                                     inspect.getfullargspec(f).args])))
            for name, f in pg_functions]
$$;

create function create_functions(code text, filename text default null, replace boolean default false) returns setof regprocedure
    language plpgsql
as
$$
declare
    rec  record;
    args text;
    fun  regprocedure;
begin
    lock table pg_proc in access exclusive mode;
    for rec in select * from omni_python.functions(code, filename)
        loop
            select
                array_to_string(array_agg(name || ' ' || type), ', ')
            from
                (select * from unnest(rec.argnames, rec.argtypes) as args(name, type)) a
            into args;
            create temporary table current_procs as (select * from pg_proc);
            execute format('create %s function %I(%s) returns %s language plpython3u as %L',
                           (case when replace then 'or replace' else '' end),
                           rec.name, args, rec.rettype, rec.code);
            select
                pg_proc.oid::regprocedure
            into fun
            from
                pg_proc
                left join current_procs on current_procs.oid = pg_proc.oid
            where
                current_procs.oid is null;
            return next fun;
            drop table current_procs;
        end loop;
    return;
end;
$$;

create function execute(code text) returns void
    language plpython3u
as
$$
    import os
    site_packages = os.path.expanduser(
        plpy.execute(plpy.prepare("select current_setting('omni_python.site_packages', true)"))[0][
            'current_setting'] or "~/.omni_python/default")
    import sys
    sys.path.insert(0, site_packages)
    del sys
    del os
    exec(compile(code, 'unnamed.py', 'exec'), globals(), locals())
$$;

create function install_requirements(requirements text) returns void
    language plpython3u
as
$$
    import io
    import contextlib
    import os
    import tempfile

    site_packages = os.path.expanduser(
        plpy.execute(plpy.prepare("select current_setting('omni_python.site_packages', true)"))[0][
            'current_setting'] or "~/.omni_python/default")

    index = plpy.execute(plpy.prepare("select current_setting('omni_python.extra_pip_index_url', true)"))[0][
        'current_setting']

    requirements_txt = tempfile.mktemp()
    with open(requirements_txt, 'w') as f:
        f.write(requirements)

    os.makedirs(site_packages, exist_ok=True)
    stderr_str = io.StringIO()
    with contextlib.redirect_stderr(stderr_str):
        try:
            from pip._internal.cli.main import main as pip
            rc = pip(["install", "--upgrade", "-r", requirements_txt, "--target", site_packages]
                     + (["--extra-index-url", index] if index is not None else []))
            if rc != 0:
                raise SystemExit(rc)
        except SystemExit as e:
            plpy.error("pip failure", detail=stderr_str.getvalue())
$$