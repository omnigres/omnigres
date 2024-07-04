create function policy_activation(_policy_id int)
    returns table
            (
                stmt text
            )
    language plpgsql
as
$pgsql$
declare
    rec record;
    subrec    record;
    target    record;
    expr record;
    filter    text;
    subfilter text;
    columns text;
    joins text;
begin
    -- For every role
    for rec in select roles.*, rolname is not null as exists
               from omni_access.roles
                        left outer join pg_authid on pg_authid.rolname = name
               where policy_id = _policy_id
        loop
            -- Ensure it exists
            if not rec.exists then
                -- No such role yet
                stmt := format('create role %I noinherit', rec.name);
                return next;

                -- Ensure no access to `public`
                stmt := format('revoke usage on schema public from %I', rec.name);
                return next;
            else
                -- Reset the role (TODO)
                stmt := format('do $$ begin raise exception $m$TODO: reset role functionality (role: %I)$m$; end $$',
                               rec.name);
                return next;
            end if;

            -- For every role:

            -- Provision the schema
            stmt := format('drop schema if exists %I', rec.name);
            return next;
            stmt := format('create schema %I', rec.name);
            return next;

            -- Ensure usage privileges for it
            stmt := format('grant usage on schema %1$I to %1$I', rec.name);
            return next;

            -- For every scope
            for subrec in select *
                          from omni_access.scopes
                                   inner join omni_access.scopes_policies on scopes_policies.policy_id = _policy_id
                loop
                    -- For every relational target in the scope
                    for target in select scope_relation_targets.*, relations.name as relation_name, relations.columns
                                  from omni_access.scope_relation_targets
                                           inner join omni_access.relations
                                                      on relations.id = scope_relation_targets.relation_id
                                                          and relations.policy_id = _policy_id
                                  where scope_id = subrec.id
                        loop

                            filter := 'true';
                            -- For every attribute expression
                            for expr in select scope_attribute_expressions.*,
                                                    attributes.expression as attribute_expression
                                             from omni_access.scope_attribute_expressions
                                                      inner join omni_access.attributes on attributes.id = attribute_id
                                             where scope_id = subrec.id
                                loop
                                    -- Add it to the filter
                                    case
                                        -- Handle `=` (equality)
                                        when expr.operator = '='
                                            then filter := filter || ' and ' || format('%L = %s', expr.expression,
                                                                                       expr.attribute_expression);
                                        -- TODO: handle the rest
                                        else raise exception 'TODO';
                                        end case;
                                end loop;

                            filter := filter;

                            -- Filter it down with `where`
                            if target."where" is not null then
                                filter := filter || ' and ' || target."where";
                            end if;

                            subfilter := '(false';

                            -- Joins.
                            joins := '';

                            -- FIXME: punt for select
                            for expr in select *
                                        from omni_access.scope_relation_rules
                                        where scope_relation_target_id = target.id
                                          and exception_for_rule_id is null
                                          and operations @> '{select}'
                                loop
                                    subfilter := subfilter || ' or ' || '/* ' || expr.name || ' */ ' ||
                                                 coalesce(expr."when", 'false');

                                    -- Add per-rule joins
                                    joins := joins ||
                                             concat_ws(' ', variadic
                                                       array(select 'left outer join ' || relation_joins.target ||
                                                                    ' on ' || relation_joins."on"
                                                             from omni_access.scope_relation_rule_joins
                                                                      inner join omni_access.relation_joins
                                                                                 on relation_joins.id = scope_relation_rule_joins.relation_join_id
                                                             where scope_relation_rule_id = expr.id
                                                       ));
                                end loop;

                            subfilter := subfilter || ')';

                            filter := filter || ' and ' || subfilter;

                            columns := concat_ws(', ', variadic array(select target.relation_name || '.' || col
                                                                      from unnest(target.columns) t(col)));


                            -- Build the view
                            stmt :=
                                    format(
                                            'create view %1$I.%2$I with (security_barrier) as select distinct %4$s from public.%2$I %5$s where %3$s',
                                            rec.name, target.relation_name, filter, columns, joins);
                            return next;

                            -- Try to grant necessary permissions
                            perform 'select' in (select unnest(target.operations));
                            if found then
                                stmt := format('grant select on %1$I.%2$I to %1$I', rec.name, target.relation_name);
                                return next;
                            end if;

                        end loop;
                end loop;

        end loop;

    return;
end;
$pgsql$;