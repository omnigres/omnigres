/*{%
  set meta_views = ["schema",
                    "cast","cast_binary_coercible","cast_function","cast_implicit","cast_implicit_in_assignment","cast_explicit",
                    "operator",
                    "sequence", "sequence_minimum_value", "sequence_start_value", "sequence_maximum_value",
                                "sequence_cycle", "sequence_increment", "sequence_type", "sequence_cache",
                                "sequence_table",
                    "relation_column","relation_column_type","relation_column_position","relation_column_default",
                                      "relation_column_nullable","relation",
                    "table","table_permanent", "table_temporary", "table_partitioned", "table_rowsecurity", "table_forcerowsecurity",
                    "view",
                    "callable", "callable_function", "callable_procedure", "callable_return_type", "callable_owner",
                                "callable_argument_name", "callable_argument_type", "callable_argument_mode", "callable_argument_default",
                                "callable_immutable", "callable_volatile", "callable_stable",
                                "callable_aggregate", "callable_window",
                                "callable_parallel_safe", "callable_parallel_unsafe", "callable_parallel_restricted",
                                "callable_security_definer", "callable_security_invoker","callable_language",
                                "callable_acl", "callable_body",
                    "trigger",
                    "role","role_inheritance",
                    "table_privilege",
                    "policy","policy_role",
                    "constraint_relation_non_null","constraint_relation_non_null_column",
                    "constraint_relation_check", "constraint_relation_check_expr",
                    "constraint_relation_unique","constraint_relation_unique_column",
                    "extension",
                    "foreign_data_wrapper","foreign_server","foreign_table","foreign_column",
                    "foreign_key",
                    "type", "type_basic", "type_array", "type_composite","type_composite_attribute",
                            "type_composite_attribute_position","type_composite_attribute_collation",
                            "type_domain","type_enum","type_enum_label", "type_pseudo", "type_range","type_multirange",
                    "index","index_relation", "index_unique", "index_unique_null_values_distinct",
                            "index_primary_key", "index_unique_immediate", "index_replica_identity",
                            "index_attribute","index_partial"
                    ]
 %}*/

create function instantiate_meta(schema name) returns void
    language plpgsql
as
$instantiate_meta$
declare
    rec record;
begin
    perform set_config('search_path', schema::text, true);

    /*{% include "../src/meta/identifiers.sql" %}*/
    /*{% include "../src/meta/identifiers-nongen.sql" %}*/
    /*{% include "../src/meta/catalog.sql" %}*/

    -- This is not perfect because of the potential pre-existing functions,
    -- but there's just so many functions there in `meta`
    for rec in select callable.*, (callable_function) is distinct from null as function
               from
                   callable
                   natural left join callable_function
                   natural left join callable_procedure
               where
                   schema_name = schema
        loop
            execute format('alter %4$s %2$I.%1$I(%3$s) set search_path to public, %2$I', rec.name, schema,
                           (select string_agg(p, ',') from unnest(rec.type_sig) t(p)),
                           case
                               when rec.function then 'function'
                               else 'procedure'
                               end
                    );
        end loop;

    /*{% include "../src/meta/create_remote_meta.sql" %}*/
    /*{% include "../src/meta/materialize_meta.sql" %}*/
    /*{% include "../src/meta/create_meta_diff.sql" %}*/

end;
$instantiate_meta$;