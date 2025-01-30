/*{%
  set meta_views = ["schema",
                    "cast","cast_binary_coercible","cast_function","cast_implicit","cast_implicit_in_assignment","cast_explicit",
                    "operator",
                    "sequence",
                    "relation_column","relation_column_type","relation_column_default",
                                      "relation_column_nullable","relation",
                    "table","table_rowsecurity", "table_forcerowsecurity",
                    "view",
                    "function","function_info_schema","function_parameter",
                    "trigger",
                    "role","role_inheritance",
                    "table_privilege",
                    "policy","policy_role",
                    "constraint_unique","constraint_check",
                    "extension",
                    "foreign_data_wrapper","foreign_server","foreign_table","foreign_column",
                    "foreign_key",
                    "type", "type_basic", "type_composite","type_composite_attribute",
                            "type_composite_attribute_position","type_composite_attribute_collation",
                            "type_domain","type_enum","type_enum_label", "type_pseudo", "type_range","type_multirange"
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

    /*{% include "../src/meta/init.sql" %}*/
    /*{% include "../src/meta/identifiers.sql" %}*/
    /*{% include "../src/meta/identifiers-nongen.sql" %}*/
    /*{% include "../src/meta/identifiers-oid.sql" %}*/
    /*{% include "../src/meta/catalog.sql" %}*/

    -- This is not perfect because of the potential pre-existing functions,
    -- but there's just so many functions there in `meta`
    for rec in select * from "function" where function.schema_name = schema
        loop
            execute format('alter function %2$I.%1$I(%3$s) set search_path to public, %2$I', rec.name, schema,
                           (select string_agg(p, ',') from unnest(rec.type_sig) t(p)));
        end loop;

    /*{% include "../src/meta/create_remote_meta.sql" %}*/
    /*{% include "../src/meta/materialize_meta.sql" %}*/
    /*{% include "../src/meta/create_meta_diff.sql" %}*/

end;
$instantiate_meta$;