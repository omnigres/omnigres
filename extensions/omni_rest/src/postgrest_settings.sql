create function postgrest_settings(schemas name[] default array []::name[])
    returns postgrest_settings
    language sql
    immutable
as
$$
select row (schemas)::omni_rest.postgrest_settings;
$$;
