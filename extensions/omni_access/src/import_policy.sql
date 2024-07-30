create function import_policy(policy jsonb) returns int
    language plpgsql as
$$
declare
    _policy_id int;
    rec record;
    subrec     record;
    arec record;
begin
    -- Import the policy record
    insert into omni_access.policies (name, version, active)
    select policy ->> 'name',
           policy ->> 'version',
           false
    returning id into _policy_id;

    -- Import roles
    insert into omni_access.roles(policy_id, name, description)
    select _policy_id,
           name,
           case
               when jsonb_typeof(value) = 'string' then value #>> '{}'
               when jsonb_typeof(value) = 'object'
                   then value ->> 'description' end description
    from jsonb_each(policy -> 'roles') policy_roles(name, value);

    -- Import relations
    for rec in insert into omni_access.relations (policy_id, name, description, relation, columns)
        select _policy_id,
               name,
               value ->> 'description',
               coalesce(value ->> 'name', name),
               -- TODO: will we have columns that are not just names?
               array(select jsonb_array_elements_text(value -> 'columns'))::text[]
        from jsonb_each(policy -> 'relations') relations(name, value)
        returning id, name
        loop
            -- Import joins
            insert
            into omni_access.relation_joins (relation_id, name, target, "on")
            select rec.id,
                   joins.name,
                   coalesce(value ->> 'target', joins.name),
                   coalesce(value ->> 'on', 'false')
            from (select *
                  from jsonb_each(policy -> 'relations' -> rec.name -> 'joins')) joins(name, value);
        end loop;

    -- Import predicates
    insert into omni_access.predicates (policy_id, name, target, "on")
    select _policy_id,
           name,
           case
               when jsonb_typeof(value) = 'string' then null
               when jsonb_typeof(value) = 'object' then value ->> 'target'
               else null end,
           case
               when jsonb_typeof(value) = 'string' then value #>> '{}'
               when jsonb_typeof(value) = 'object' then value ->> 'on'
               else '' end
    from jsonb_each(policy -> 'predicates') predicates(name, value);

    -- Import scopes
    for rec in
        with recursive
            scopes as materialized (select nextval('omni_access.scopes_id_seq') as id,
                                           name,
                                           value,
                                           null::bigint                         as parent_id
                                    from jsonb_each(policy -> 'scopes') t(name, value)
                                    union all
                                    select nextval('omni_access.scopes_id_seq') as id,
                                           t.name,
                                           t.value,
                                           scopes.id
                                    from scopes,
                                         jsonb_each(scopes.value -> 'scopes') t(name, value)),
            _insertions as (insert into omni_access.scopes (id, name, description, parent_id)
                overriding system value -- so that we can supply the id
                select id,
                       name,
                       value ->> 'description',
                       parent_id
                from scopes
                returning id, name, parent_id)
        select *
        from scopes
        loop
            -- Attach scope to the policy if it is top level
            if rec.parent_id is null then
                insert into omni_access.scopes_policies (scope_id, policy_id) values (rec.id, _policy_id);
            end if;

            -- Set roles
            insert into omni_access.scope_roles (scope_id, role_id)
            select rec.id, roles.id
            from jsonb_array_elements_text(case
                                               when jsonb_typeof(rec.value -> 'roles') = 'string'
                                                   then jsonb_build_array(policy -> 'scopes' -> rec.name -> 'roles')
                                               else policy -> 'scopes' -> rec.name -> 'roles' end) value
                     inner join omni_access.roles on roles.name = value;

            -- Import predicate applications
            insert into omni_access.scope_predicates (scope_id, predicate_id)
                (select rec.id, predicates.id
                 from jsonb_array_elements_text(rec.value -> 'predicates') pred(name)
                          inner join omni_access.predicates
                                     on predicates.policy_id = _policy_id and predicates.name = pred.name);

            -- Scope's relations
            for subrec in select * from jsonb_each(rec.value -> 'relations') relations(name, value)
                loop
                    declare
                        _relation_id int;
                        _target_id int;
                    begin
                        -- Does the relation exist?
                        select id from omni_access.relations where name = subrec.name into _relation_id;
                        if not found then
                            raise exception 'Relation % is not defined', subrec.name;
                        end if;

                        -- Add relation target
                        for arec in with targets
                                             as materialized (select nextval('omni_access.scope_relation_targets_id_seq') as id,
                                                                     rec.id                                               as scope_id,
                                                                     _relation_id as rel_id,
                                                                     subrec.value                                         as value),
                                         _insertions as (insert
                                             into omni_access.scope_relation_targets (id, scope_id, relation_id, "where", operations)
                                                 overriding system value
                                                 select id,
                                                        scope_id,
                                                        rel_id,
                                                        value ->>
                                                        'where',
                                                        array(select jsonb_array_elements_text(value -> 'operations'))::omni_access.relation_operation[]
                                                 from targets)
                                    select *
                                    from targets
                            loop

                                _target_id := arec.id;

                                -- For every target, add joins
                                if jsonb_typeof(arec.value -> 'joins') = 'array' then
                                    insert
                                    into omni_access.scope_relation_target_joins (scope_relation_target_id, relation_join_id)
                                    select arec.id, relation_joins.id
                                    from omni_access.relation_joins,
                                         (select * from jsonb_array_elements_text(arec.value -> 'joins') t(name)) joins
                                    where _relation_id = arec.rel_id
                                      and relation_joins.name = joins.name;
                                end if;
                            end loop;

                        -- Rules
                        for arec in
                            with recursive
                                rules
                                    as materialized (select nextval('omni_access.scope_relation_rules_id_seq') as id,
                                                            _target_id                                         as target_id,
                                                            name,
                                                            value,
                                                            null::bigint                                       as parent_id,
                                                            _relation_id as rel_id
                                                     from jsonb_each(subrec.value -> 'rules') rules(name, value)
                                                     union all
                                                     select nextval('omni_access.scope_relation_rules_id_seq') as id,
                                                            _target_id                                         as target_id,
                                                            t.name,
                                                            t.value,
                                                            rules.id                                           as parent_id,
                                                            _relation_id as rel_id
                                                     from rules,
                                                          jsonb_each(rules.value -> 'except') t(name, value)),
                                _insertion as (insert
                                    into omni_access.scope_relation_rules (id, scope_relation_target_id, name,
                                                                           description,
                                                                           "when", exception_for_rule_id, operations)
                                        overriding system value
                                        select id,
                                               target_id,
                                               name,
                                               value ->> 'description',
                               value ->> 'when',
                               parent_id,
                               array(select distinct jsonb_array_elements_text(
                                                             coalesce(value -> 'operations', subrec.value -> 'operations')))::omni_access.relation_operation[]
                                        from rules)
                            select *
                            from rules
                            loop
                                -- For every rule, add joins
                                if jsonb_typeof(arec.value -> 'joins') = 'array' then
                                    insert
                                    into omni_access.scope_relation_rule_joins (scope_relation_rule_id, relation_join_id)
                                    select arec.id, relation_joins.id
                                    from omni_access.relation_joins,
                                         (select * from jsonb_array_elements_text(arec.value -> 'joins') t(name)) joins
                                    where relation_joins.relation_id = arec.rel_id
                                      and relation_joins.name = joins.name;
                                end if;
                            end loop;
                    end;
                end loop;

            -- TODO: functions?
        end loop;

    -- Done
    return _policy_id;
end;
$$;