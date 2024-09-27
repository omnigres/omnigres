create function policy_activation(_policy_id int)
    returns table
            (
                stmt text
            )
    language plpgsql
as
$pgsql$
declare
    role_rec         record;
    relation_rec     record;
    predicate_rec    record;
    op_rec           record;
    rule_rec record;
    join_rec record;
    filter           text[];
    columns text;
    joins text;
begin
    -- For every role
    for role_rec in select roles.*, rolname is not null as exists
               from omni_access.roles
                        left outer join pg_authid on pg_authid.rolname = name
               where policy_id = _policy_id
        loop
            -- Ensure it exists
            if not role_rec.exists then
                -- No such role yet
                stmt := format('create role %I noinherit', role_rec.name);
                return next;

                -- Ensure no access to `public`
                stmt := format('revoke usage on schema public from %I', role_rec.name);
                return next;
            else
                -- Reset the role (TODO)
                stmt := format('do $$ begin raise exception $m$TODO: reset role functionality (role: %I)$m$; end $$',
                               role_rec.name);
                return next;
            end if;

            -- For every role:

            -- Provision the schema
            stmt := format('drop schema if exists %I', role_rec.name);
            return next;
            stmt := format('create schema %I', role_rec.name);
            return next;

            -- Ensure usage privileges for it
            stmt := format('grant usage on schema %1$I to %1$I', role_rec.name);
            return next;

            -- Now, rather somewhat counter-intuitively, we need to process every relation even though these are listed
            -- inside of scopes. The reason behind that is that we effectively inverted the way we create queries: instead
            -- of going for large queries, we slice them according to scopes. However, the final product is just one giant
            -- [view] over a relation. So it makes more sense to attend one query at a time, holistically.

            for relation_rec in with recursive scopes as (select scopes.*
                                                          from omni_access.scopes
                                                                   inner join omni_access.scopes_policies
                                                                              on scopes_policies.policy_id =
                                                                                 1 and
                                                                                 scopes_policies.scope_id =
                                                                                 scopes.id
                                                          where parent_id is null
                                                          union all
                                                          select child.*
                                                          from omni_access.scopes child
                                                                   inner join scopes parent on parent.id = child.parent_id)
                                select srt.relation_id,
                                       r.name                                                                   as relation_name,
                                       srt."where",
                                       r.columns,
                                       array_agg(distinct (p))                                                  as predicates,
                                       array_agg(distinct (srr))                                                as rules,
                                       array_agg(distinct (j))
                                       filter (where srtj.scope_relation_target_id is not null) as joins,
                                       array_agg(distinct srr.operations)
                                       filter (where srr.operations <> '{}')                                    as operations
                                from scopes
                                         inner join omni_access.scope_relation_targets srt on srt.scope_id = scopes.id
                                         inner join omni_access.scope_predicates sp on sp.scope_id = scopes.id
                                         inner join omni_access.predicates p on sp.predicate_id = p.id
                                         left join omni_access.scope_relation_rules srr
                                                   on srr.scope_relation_target_id =
                                                      srt.id
                                         left join omni_access.scope_relation_target_joins srtj
                                                   on srtj.scope_relation_target_id = srt.id
                                         left join omni_access.relation_joins j on j.id = srtj.relation_join_id
                                         inner join omni_access.relations r on r.id = srt.relation_id
                                group by srt.relation_id, srt."where", r.name, r.columns
                loop
                    joins := '';

                    filter := '{}'::text[];

                    -- Predicates
                    --
                    -- FIXME: it is a little suspect that we're going over predicates here. What happens with scopes
                    -- without predicates? Can such a thing exists? If not, should the database enforce this domain model
                    -- requirement?
                    for predicate_rec in select 'left join ' ||
                                                coalesce('public.' || target, '(select true as ctid)') ||
                                                ' as ' ||
                                                name || ' on ' || "on" as join_expr,
                                                name
                                         from unnest(relation_rec.predicates)
                                         union
                                         select null, null
                                         where cardinality(relation_rec.predicates) = 0
                        loop
                            declare
                                where_clauses text[];
                                where_filter text;
                            begin
                                where_clauses := '{}';
                                -- Process `where` conditions imposed by the rules
                                for rule_rec in select *
                                                from unnest(relation_rec.rules)
                                                where exception_for_rule_id is null
                                    loop
                                        where_clauses := where_clauses || rule_rec."when";
                                    end loop;
                                -- TODO: handle exceptions (`exception_for_rule_id`)

                                where_filter := null;

                                -- Process `where` condition imposed by the relation
                                if relation_rec."where" is not null then
                                    where_filter := relation_rec."where";
                                end if;

                                if cardinality(where_clauses) > 0 then
                                    where_filter :=
                                            case when where_filter is null then '' else where_filter || ' and ' end ||
                                            '(' || concat_ws(' or ', variadic where_clauses) ||
                                            ')';
                                end if;

                                -- Inject predicate clauses
                                if predicate_rec.name is not null then
                                    joins := joins || ' ' || predicate_rec.join_expr;
                                    filter := filter ||
                                              ('(' || predicate_rec.name || '.ctid is not null and (' || where_filter ||
                                               '))')::text;
                                else
                                    filter := filter || ('(' || where_filter || ')');
                                end if;

                                -- Inject joins
                                for join_rec in select * from unnest(relation_rec.joins)
                                    loop

                                    end loop;


                                joins := joins ||
                                         concat_ws(' ', variadic
                                                   array(select '/* HI */ left join ' ||
                                                                relation_joins.target || ' ' ||
                                                                relation_joins.name ||
                                                                ' on ' ||
                                                                relation_joins."on"
                                                         from omni_access.relation_joins
                                                         where relation_joins.id in (select id
                                                                                     from unnest(relation_rec.joins))));
                            end;
                        end loop;


                    -- Prepare the columns
                    columns := concat_ws(', ', variadic array(select relation_rec.relation_name || '.' || col
                                                              from unnest(relation_rec.columns) t(col)));
                    -- Build the view
                    stmt :=
                            format(
                                    'create view %1$I.%2$I with (security_barrier) as select distinct %4$s from public.%2$I %5$s where %3$s',
                                    role_rec.name, relation_rec.relation_name, concat_ws(' or ', variadic filter),
                                    columns, joins);
                    return next;

                    for op_rec in select value from unnest(relation_rec.operations) t(value)
                        loop
                            stmt :=
                                    format('grant %3$s on %1$I.%2$I to %1$I', role_rec.name, relation_rec.relation_name,
                                           op_rec.value);
                            return next;
                        end loop;
                end loop;

/*
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


                            -- Start preparing joins.
                            joins := '';

                            filter := 'true';
                            --                             -- For every attribute expression
--                             for expr in select scope_attribute_expressions.*,
--                                                     attributes.expression as attribute_expression
--                                              from omni_access.scope_attribute_expressions
--                                                       inner join omni_access.attributes on attributes.id = attribute_id
--                                              where scope_id = subrec.id
--                                 loop
--                                     if filter != '' then
--                                         filter := filter || ' and ';
--                                     end if;
--                                     -- Add it to the filter
--                                     case
--                                         -- Handle `=` (equality)
--                                         when expr.operator = '='
--                                             then filter := filter || format('%L = %s', expr.expression,
--                                                                                        expr.attribute_expression);
--                                         -- TODO: handle the rest
--                                         else raise exception 'TODO';
--                                         end case;
--                                 end loop;

                            filter := filter;

                            -- Filter it down with `where`
                            if target."where" is not null then
                                filter := filter || ' and ' || target."where";
                            end if;

                            subfilter := '(';


                            -- FIXME: punt for select
                            for expr in select *
                                        from omni_access.scope_relation_rules
                                        where scope_relation_target_id = target.id
                                          and exception_for_rule_id is null
                                          and operations @> '{select}'
                                loop
                                    if subfilter != '(' then
                                        subfilter := subfilter || ' or ';
                                    end if;
                                    subfilter := subfilter || '/* ' || expr.name || ' */ ' ||
                                                 coalesce(expr."when", 'false');

                                    -- Add per-rule joins
                                    joins := joins ||
                                             concat_ws(' ', variadic
                                                       array(select 'left join public.' ||
                                                                    relation_joins.target || ' ' ||
                                                                    relation_joins.name ||
                                                                    ' on ' ||
                                                                    relation_joins."on"
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
*/
        end loop;

    return;
end;
$pgsql$;