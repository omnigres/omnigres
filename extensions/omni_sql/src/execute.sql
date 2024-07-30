create or replace function execute(stmt text, parameters jsonb default '[]',
                                   types regtype[] default '{}'::regtype[])
    returns table
            (
                stmt_row jsonb
            )
as
$pgsql$
declare
    rec            record;
    retrec         record;
    exec_stmt text;
begin
    if jsonb_typeof(parameters) != 'array' then
        raise exception 'Parameters should be a JSONB array';
    end if;
    for rec in select source                                              as stmt,
                      coalesce(not lead(true) over (), true)              as last
               from omni_sql.raw_statements(stmt::cstring)
        loop
            if not rec.last then
                execute rec.stmt;
            else
                execute
                    'prepare _omni_sql_prepared_statement ' ||
                    case
                        when cardinality(types) = 0 then ''
                        else '(' || concat_ws(', ', variadic types::text[]) ||
                             ')'
                        end || ' as ' || rec.stmt;
                begin
                    exec_stmt :=
                            format('execute _omni_sql_prepared_statement ' ||
                                   case
                                       when jsonb_array_length(parameters) = 0 then ''
                                       else '(' ||
                                            concat_ws(', ', variadic
                                                      (select array_agg(format('%L', param))
                                                       from jsonb_array_elements_text(parameters) t(param))) ||
                                            ')' end);
                    if omni_sql.is_returning_statement(rec.stmt::omni_sql.statement) then
                        for retrec in execute exec_stmt
                            loop
                                stmt_row := to_jsonb(retrec);
                                return next;
                            end loop;
                    else
                        declare
                            stmt_row_count bigint;
                        begin
                            execute exec_stmt;
                            -- doesn't currently work for utility statements
                            -- due to a [potential] bug in SPI
                            get diagnostics stmt_row_count = row_count;
                            stmt_row := jsonb_build_object('rows', stmt_row_count);
                            return next;
                        end;
                    end if;
                    execute 'deallocate _omni_sql_prepared_statement';
                exception
                    when others then
                        execute 'deallocate _omni_sql_prepared_statement';
                        raise;
                end;
            end if;
        end loop;
    return;
end;
$pgsql$
    language plpgsql strict;