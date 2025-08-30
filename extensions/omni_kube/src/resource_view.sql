create function resource_view(view_name name, group_version text, resource text, label_selector text default null,
                              field_selector text default null)
    returns regclass
    language plpgsql
as
$resource_view$
declare
    ns              name := current_setting('omni_kube.search_path');
    url             text;
    is_namespaced   boolean;
    resource_kind   text;
    old_search_path text := current_setting('search_path');
begin
    execute format('set search_path to %I, public', ns);
    select namespaced, kind into is_namespaced, resource_kind from group_resources(group_version) where name = resource;
    url := resources_path(group_version, resource, namespace => case when is_namespaced then '%s' end);
    perform
        set_config('search_path', old_search_path, true);


    execute format($resource_view_$
    create or replace view %I as
    select
     resource->'metadata'->>'uid' as uid,
     resource->'metadata'->>'name' as name,
     resource->'metadata'->>'namespace' as namespace,
     * from %I.resources(%L, %L, label_selector => %L, field_selector => %L) resource
    $resource_view_$, view_name, ns, group_version, resource, label_selector, field_selector);

    execute format($resource_view_insert$
    create or replace function %I() returns trigger
    set search_path to %I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name text;
      url text;
      metadata jsonb;
      result jsonb;
    begin
      if new.uid is not null then
         raise exception 'new resources can''t have uid';
      end if;
      metadata := coalesce(new.resource->'metadata','{"namespace": "default"}'::jsonb);
      spec_ns := coalesce(metadata->>'namespace', 'default');
      if new.namespace is not null and spec_ns != new.namespace then
        raise exception 'namespace (%%) must match metadata (%%)', new.namespace, spec_ns;
      end if;
      name := coalesce(coalesce(metadata->>'name', metadata->>'generateName'), new.name);
      if name is null then
        raise exception 'resource is required to have `name` or `generateName`';
      end if;
      if new.name is not null and name != new.name then
        raise exception 'name (%%) must match metadata (%%)', new.name, name;
      end if;
      new.resource := jsonb_set(new.resource, '{kind}', to_jsonb(%L::text));
      new.resource := jsonb_set(new.resource, '{apiVersion}', to_jsonb(%L::text));
      url := format(%L, spec_ns);
      select api(url, body => new.resource, method => 'POST') into result;
      new.uid := result->'metadata'->>'uid';
      new.name := result->'metadata'->>'name';
      new.namespace := result->'metadata'->>'namespace';
      new.resource := result;
      return new;
    end;
    $$
    $resource_view_insert$, view_name || '_insert', ns, resource_kind, group_version, url);

    execute format($resource_view_insert_t$
    create trigger %1$I instead of insert on %2$I for each row
    execute function %1$I()
    $resource_view_insert_t$, view_name || '_insert', view_name);

    execute format($resource_view_update$
    create or replace function %I() returns trigger
    set search_path to %I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name text;
      url text;
      result jsonb;
    begin
      spec_ns := coalesce(coalesce(new.resource->'metadata','{"namespace": "default"}'::jsonb)->>'namespace', 'default');
      if new.namespace is not null and spec_ns != new.namespace then
        raise exception 'namespace (%%) must match metadata (%%)', new.namespace, spec_ns;
      end if;
      name := coalesce(coalesce(new.resource->'metadata','{}')->>'name', new.name);
      if name is null then
        raise exception 'resource is required to have a name';
      end if;
      if new.name is not null and name != new.name then
        raise exception 'name (%%) must match metadata (%%)', new.name, name;
      end if;
      url := format(%L, spec_ns);
      select api(url || '/' || name, body => new.resource, method => 'PUT') into result;
      new.resource := result;
      return new;
    end;
    $$
    $resource_view_update$, view_name || '_update', ns, url);

    execute format($resource_view_update_t$
    create trigger %1$I instead of update on %2$I for each row
    execute function %1$I()
    $resource_view_update_t$, view_name || '_update', view_name);

    execute format($resource_view_delete$
    create or replace function %I() returns trigger
    set search_path to %I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name text;
      url text;
    begin
      spec_ns := coalesce(coalesce(old.resource->'metadata','{"namespace": "default"}'::jsonb)->>'namespace', 'default');
      if old.namespace is not null and spec_ns != old.namespace then
        raise exception 'namespace (%%) must match metadata (%%)', old.namespace, spec_ns;
      end if;
      name := coalesce(coalesce(old.resource->'metadata','{}')->>'name', old.name);
      if name is null then
        raise exception 'resource is required to have a name';
      end if;
      if old.name is not null and name != old.name then
        raise exception 'name (%%) must match metadata (%%)', old.name, name;
      end if;
      url := format(%L, spec_ns);
      perform api(url || '/' || name, method => 'DELETE');
      return old;
    end;
    $$
    $resource_view_delete$, view_name || '_delete', ns, url);

    execute format($resource_view_delete_t$
    create trigger %1$I instead of delete on %2$I for each row
    execute function %1$I()
    $resource_view_delete_t$, view_name || '_delete', view_name);

    return view_name;
end;
$resource_view$;
