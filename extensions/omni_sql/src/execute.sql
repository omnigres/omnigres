create function execute(stmt text, parameters jsonb default NULL,
                        types regtype[] default NULL::regtype[])
    returns table
            (
                stmt_row jsonb
            )
as
$pgsql$
declare
    rec            record;
    retrec         record;
begin
    for rec in select source                                              as stmt,
                      coalesce(not lead(true) over (), true)              as last
               from omni_sql.raw_statements(stmt::cstring)
        loop
            if not rec.last then
                execute rec.stmt;
            else
                for retrec in select omni_sql.execute_parameterized(rec.stmt, parameters, types) val
                    loop
                        stmt_row := to_jsonb(retrec.val);
                        return next;
                    end loop;
            end if;
        end loop;
    return;
end;
$pgsql$
    language plpgsql;


comment on function omni_sql.execute is 'Runs an arbitrary SQL statement. If the statement is a parameterized statement, one can define more precise types for the statement using the "types" parameter.'
