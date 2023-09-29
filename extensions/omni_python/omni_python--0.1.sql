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

    module = ast.parse(code)

    # Load imports and classes so we can resolve annotations
    code_locals = {}
    code_globals = globals()
    for f in module.body:
        if f.__class__ == ast.Import or f.__class__ == ast.ImportFrom or f.__class__ == ast.ClassDef:
            exec(compile(ast.unparse(f), filename or 'unnamed.py', 'single'), code_globals, code_locals)

    #pg_functions = [f for f in module.body if f.__class__ == ast.FunctionDef
    #                for dec in f.decorator_list if dec.__class__ == ast.Name and dec.id == 'pg']

    #for f in pg_functions:
    #    f.decorator_list = [dec for dec in f.decorator_list if not (dec.__class__ == ast.Name and dec.id == 'pg')]
    pg_functions = []
    for f in module.body:
        if isinstance(f, ast.FunctionDef):
            code_locals_ = code_locals.copy()
            exec(compile(ast.unparse(f), filename or 'unnamed.py', 'single'), code_globals, code_locals_)
            function = code_locals_[f.name]
            if hasattr(function, '__pg_stored_procedure__'):
                pg_functions.append(f)

    __types__ = {str: 'text', bool: 'boolean', bytes: 'bytea', int: 'int',
                 decimal.Decimal: 'numeric', float: 'double precision'}

    def resolve_type(f, arg):
        # Eval the function definition to resolve annotations
        exec(compile(ast.unparse(f), filename or 'unnamed.py', 'single'), code_globals, code_locals)
        function = code_locals[f.name]
        type = function.__annotations__[arg]
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

    site_packages = plpy.quote_literal(
        os.path.expanduser(plpy.execute(plpy.prepare("select current_setting('omni_python.site_packages', true)"))[0][
                               'current_setting'] or "~/.omni_python/default"))

    preamble = f"import sys ; sys.path.insert(0, {site_packages}) ; del sys"
    return [(f.name,
             [a.arg for a in f.args.args],
             [resolve_type(f, a.arg) for a in f.args.args], resolve_type(f, 'return'),
             "{preamble}\n{code}\nreturn {name}({args})".format(preamble=preamble, code=ast.unparse(module),
                                                                name=f.name,
                                                                args=', '.join([a.arg for a in f.args.args])))
            for f in pg_functions]
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