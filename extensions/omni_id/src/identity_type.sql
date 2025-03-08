create function identity_type(name name, type regtype default 'int8',
                              sequence text default null, create_sequence boolean default true,
                              increment bigint default 1,
                              minvalue bigint default null,
                              maxvalue bigint default null,
                              cache bigint default null,
                              cycle boolean default null,
                              constructor text default null,
                              create_constructor bool default true,
                              operator_schema name default 'public',
                              nextval regprocedure default null
) returns regtype
    language plpgsql
as
$$
declare
    rec record;
    type_name text;
    ns name;
begin
    -- Remember current namespace
    select current_schema() into ns;

    type_name := type::text;
    -- Ensure we only allow supported types
    case type_name
        when 'bigint' then type_name := 'int8';
        when 'int','integer' then type_name := 'int4';
        when 'smallint' then type_name := 'int2';
        else
        end case;
    if not type_name = any ('{int2,int4,int8,uuid}'::text[]) then
        raise exception 'type must be smallint, int, bigint or uuid';
    end if;
    -- Ensure this type hasn't been declared already
    for rec in select *, (case when type_name = 'uuid' then 'uuid' else 'int' || typlen end) as base_type
               from pg_type
                        inner join pg_namespace on nspname = current_schema and typnamespace = pg_namespace.oid
               where typname = name
        loop
        -- If the type exists and matches the following, we just return it:
        -- 0. Schema (namespace) [we already filter for that in the query above]
        -- 1. Length (base_type = type)
        -- TODO: Should we constrain it any further?
            if rec.base_type = type_name then
                return name::regtype;
            end if;
            -- Otherwise, error out
            raise exception 'Type % already exists', name;
        end loop;

    -- Define the shell of the type
    execute format('create type %I;', name);

    -- Make sure the commands below don't produce these messages:
    -- NOTICE:  return type a is only a shell
    -- NOTICE:  argument type a is only a shell
    -- NOTICE:  argument type a is only a shell
    -- NOTICE:  return type a is only a shell
    --
    -- (they are annoying and unnecessary)
    set local client_min_messages = warning;

    -- Define in/out send/recv before we actually define the type
    -- (piggying back on the underlying type)
    execute format('create function %I(cstring) returns %I language internal as %L immutable strict parallel safe',
                   name || '_in',
                   name,
                   (case when type_name = 'uuid' then 'uuid_in' else type_name || 'in' end));
    execute format('create function %I(%I) returns cstring language internal as %L immutable strict parallel safe',
                   name || '_out', name,
                   (case when type_name = 'uuid' then 'uuid_out' else type_name || 'out' end));
    execute format('create function %I(%I) returns bytea language internal as %L immutable strict parallel safe',
                   name || '_send', name,
                   (case when type_name = 'uuid' then 'uuid_send' else type_name || 'send' end));
    execute format('create function %I(internal) returns %I language internal as %L immutable strict parallel safe',
                   name || '_recv', name,
                   (case when type_name = 'uuid' then 'uuid_recv' else type_name || 'recv' end));

    -- Resume notices disabled above
    reset client_min_messages;

    -- Finally, define the type
    execute format(
            'create type %I ( internallength = %s, input = %I, output = %I, like = %I, receive = %I, send = %I, storage = plain)',
            name, (select typlen from pg_type where typname = type_name), name || '_in', name || '_out', type_name,
            name || '_recv', name || '_send'
            );

    -- Define casts in and out of the type
    execute format('create cast (%I as %I) without function', type_name, name);
    execute format('create cast (%I as %I) without function', name, type_name);

    -- Define binary operators
    for rec in select *
               from (values ('=', 'eq'),
                            ('>', 'gt'),
                            ('<', 'lt'),
                            ('>=', 'ge'),
                            ('<=', 'le'),
                            ('<>', 'ne')) operators(op, name)
        loop
            -- Define the function, again piggying back on the underlying type
            execute format(
                    'create function %I(%2$I, %2$I) returns boolean language internal as %L immutable strict parallel safe',
                           name || '_' || rec.name, name,
                           (case when type_name = 'uuid' then 'uuid_' else type_name end) || rec.name);

            -- Define the actual operator
            execute format('create operator %4$I.%1$s (
                                         leftarg = %2$I,
                                         rightarg = %2$I,
                                         procedure = %3$I)', rec.op, name, name || '_' || rec.name,
                           coalesce(operator_schema, 'public'));

        end loop;

    -- Define a `cmp` function (again, piggy back on the underlying type)
    execute format('create function %I(%I,%2$I) returns int language internal as %3$L immutable strict parallel safe',
                   name || '_cmp', name,
                   (case when type_name = 'uuid' then 'uuid_cmp' else 'bt' || type_name || 'cmp' end));

    -- Define btree operator class
    execute format('create operator class %I
        default for type %I using btree AS
        operator 1 <,
        operator 2 <=,
        operator 3 =,
        operator 4 >=,
        operator 5 >,
       function 1 %I', name || '_ops', name, name || '_cmp');

    if type_name = 'uuid' then
        create_sequence := false;
    end if;

    if create_sequence then
        -- If requested, create a sequence
        if sequence is null then
            sequence := name || '_seq';
        end if;
        execute format('create sequence %I as %I', sequence, type_name);
        -- Define `name`_nextval() function to return next value in the given sequence
        --
        -- We are doing double-casting because nextval() itself always returns bigint
        execute format(
                'create function %I() returns %I language sql as $sql$ select nextval(%L)::%4$I::%5$I.%2$I $sql$',
                name || '_nextval', name, ns || '.' || sequence, type_name, ns);

        -- Define `name`_currval() to return current value in the given sequence
        -- (same double-casting)
        execute format(
                'create function %I() returns %I language sql as $sql$ select currval(%L)::%4$I::%5$I.%2$I $sql$',
                name || '_currval', name, ns || '.' || sequence, type_name, ns);

        -- Define `name`_setval() to return current value in the given sequence
        -- (same double-casting)
        execute format(
                'create function %1$I(value %2I, is_called boolean default true) returns %2$I language sql as $sql$ select setval(%3$L, value::%5$I.%2$I::bigint, is_called)::%5$I.%2$I $sql$ strict',
                name || '_setval', name, ns || '.' || sequence, type_name, ns);

        -- Process options
        if increment is not null then
            execute format('alter sequence %I increment %s', sequence, increment);
        end if;

        if minvalue is not null then
            execute format('alter sequence %I start %2$s restart %2$s minvalue %2$s', sequence, minvalue);
        end if;

        if maxvalue is not null then
            execute format('alter sequence %I maxvalue %s', sequence, maxvalue);
        end if;

        if cache is not null then
            execute format('alter sequence %I cache %s', sequence, cache);
        end if;

        if cycle is not null then
            execute format('alter sequence %I cycle %s', sequence, cycle);
        end if;

    end if;

    -- If sequence API is desired for UUID, supply it as `_nextval`
    if not create_sequence and nextval is not null then
        execute format(
                'create function %I() returns %I return %s()::%4$I.%2$I',
                name || '_nextval', name, nextval::regproc, ns);
    end if;


    if create_constructor then
        if constructor is null then
            constructor := name;
        end if;

        execute format(
                'create function %1$I(value %2$I) returns %3$I  immutable strict parallel safe return value::%4$I.%3$I',
                constructor, type_name, name, ns);

    end if;

    return name::regtype;
end;
$$;
