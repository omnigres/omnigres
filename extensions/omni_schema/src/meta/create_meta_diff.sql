create function create_meta_diff(
    baseline regnamespace,
    changed regnamespace,
    schema regnamespace
) returns void
    language plpgsql
as
$create_meta_diff$
declare
    types text[] :=
        array[
            'cast',
            'column',
            'connection',
            'constraint_check',
            'constraint_unique',
            'extension',
            'foreign_column',
            'foreign_data_wrapper',
            'foreign_key',
            'foreign_server',
            'foreign_table',
            'function',
            'operator',
            'policy',
            'relation',
            'relation_column',
            'role',
            'role_inheritance',
            'schema',
            'sequence',
            'table',
            'table_privilege',
            'trigger',
            'type',
            'view'
            ];
    type  text;
begin
    perform set_config('search_path', schema::text, true);

    foreach type in array types
        loop
            execute format($sql$
    create or replace view added_%3$s as select b.* from %2$I.%3$I b
        left outer join %1$I.%3$I a on a.id = b.id
        where a is not distinct from null
    $sql$, baseline, changed, type);

            execute format($sql$
    create or replace view removed_%3$s as select b.* from %1$I.%3$I b
        left outer join %2$I.%3$I a on a.id = b.id
        where a is not distinct from null
    $sql$, baseline, changed, type);

            execute format($sql$
    create or replace view baseline_%3$s as select a.* from %2$I.%3I b
        inner join %1$I.%3$I a on a.id = b.id
        where a != b
    $sql$, baseline, changed, type);

            execute format($sql$
    create or replace view changed_%3$s as select b.* from %2$I.%3I b
        inner join %1$I.%3$I a on a.id = b.id
        where a != b
    $sql$, baseline, changed, type);

        end loop;

    return;
end;
$create_meta_diff$;