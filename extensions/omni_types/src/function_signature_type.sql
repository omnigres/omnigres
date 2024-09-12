create function function_signature_type(name name,
                                        variadic signature regtype[]
) returns regtype
    language plpgsql
as
$$
declare
    ret_type           regtype;
    omni_types_library text;
    rec record;
begin

    for rec in select typ, arguments, return from omni_types.function_signature_types where typ::name = name
        loop
            declare
                existing_sig regtype[];
            begin
                existing_sig := trim_array(array_append(rec.arguments, rec.return), 0);
                if not array_eq(existing_sig, signature) then
                    raise exception 'cannot redefine % as its signature % does not match %', name, existing_sig, signature;
                else
                    return rec.typ;
                end if;
            end;
        end loop;

    -- Resolve omni_types module
    select probin
    into omni_types_library
    from pg_proc
             inner join pg_namespace on pg_proc.pronamespace = pg_namespace.oid and pg_namespace.nspname = 'omni_types'
    where proname = 'unit_in';

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

    execute format('create function %I(cstring) returns %I language c as %L, %L immutable strict', name || '_in',
                   name,
                   omni_types_library, 'function_signature_in');

    execute format('create function %I(%I) returns cstring language internal as %L immutable strict', name || '_out',
                   name,
                   'regprocedureout');

    execute format('create function %I(%I) returns bytea language internal as %L immutable',
                   name || '_send', name,
                   'regproceduresend');
    execute format('create function %I(internal) returns %I language internal as %L immutable',
                   name || '_recv', name,
                   'regprocedurerecv');

    -- Resume notices disabled above
    reset client_min_messages;

    -- Finally, define the type
    execute format(
            'create type %I ( internallength = %s, input = %I, output = %I, send = %I, receive = %I, like = oid, storage = plain)',
            name, (select typlen from pg_type where typname = 'oid'), name || '_in', name || '_out', name || '_send',
            name || '_recv'
            );

    -- Register the type
    ret_type := signature[cardinality(signature)];
    insert into omni_types.function_signature_types (typ, arguments, return)
    values (name::regtype, trim_array(signature, 1), ret_type);

    -- Define casts out of the type
    execute format('create cast (%I as regproc) without function', name);
    execute format('create cast (%I as regprocedure) without function', name);
    execute format('create cast (%I as oid) without function', name);

    -- Define conformance test function
    execute format('create function %3$I(text) returns %1$I language c as %2$L, ''conforming_function''',
                   name,
                   omni_types_library, name || '_conforming_function');

    -- Define caller function
    execute format('create function %I(%I %s) returns %s language c as %L, %L', 'call_' || name, name,
                   case
                       when cardinality(signature) = 1 then ''
                       else ', ' || concat_ws(', ', variadic trim_array(signature, 1)) end,
                   ret_type, omni_types_library, 'invoke');

    return name::regtype;
end;
$$;