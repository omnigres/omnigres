create function resource_table(table_name name, group_version text, resource text, label_selector text default null,
                               field_selector text default null)
    returns regclass
    language plpgsql
as
$resource_table$
declare
    ns              name := current_setting('omni_kube.search_path');
    url             text;
    is_namespaced   boolean;
    resource_kind   text;
    old_search_path text := current_setting('search_path');
begin
    execute format('set search_path to %I, public', ns);
    select namespaced, kind into is_namespaced, resource_kind from group_resources(group_version) where name = resource;
    perform
        set_config('search_path', old_search_path, true);

    if is_namespaced then
        url := '/' ||
               case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end || '/namespaces/%s/' ||
               resource;
    else
        url := '/' ||
               case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end || '/' ||
               resource;
    end if;
    execute format($resource_table_$
    create table %I as
    select
     resource->'metadata'->>'uid' as uid,
     resource->'metadata'->>'name' as name,
     resource->'metadata'->>'namespace' as namespace,
     * from %I.resources(%L, %L, label_selector => %L, field_selector => %L) resource
    $resource_table_$, table_name, ns, group_version, resource, label_selector, field_selector);

    execute format($$create unique index %2$I on %1$I (uid) $$, table_name, table_name || '_index_uid');
    execute format(
            $$create unique index %2$I on %1$I (name, namespace) $$,
            table_name,
            table_name || '_index_name_space');

    execute format($resource_table_refresh$
    create function %1$I() returns table (type text, object jsonb)
    set omni_kube.refresh = true
    begin atomic;
     with new as materialized (select
     resource->'metadata'->>'uid' as uid,
     resource->'metadata'->>'name' as name,
     resource->'metadata'->>'namespace' as namespace,
     resource from %3$I.resources(%4$L, %5$L, label_selector => %6$L, field_selector => %7$L) resource),
       deletion as (delete from %2$I where not exists (select from new where new.uid = %2$I.uid) returning %2$I.*),
       insertion as (insert into %2$I select * from new where not exists (select from %2$I x where x.uid = new.uid) returning %2$I.*),
       updating as (update %2$I set uid = new.uid, name = new.name, namespace = new.namespace, resource = new.resource
          from new where new.uid = %2$I.uid and (new) != (%2$I) returning %2$I.*)
       select 'DELETED' as type, od.resource as object from deletion od
       union all
       select 'ADDED' as type, oi.resource as object from insertion oi
       union all
       select 'MODIFIED' as type, ou.resource as object from updating ou;
    end;
    $resource_table_refresh$, 'refresh_' || table_name, table_name, ns, group_version, resource, label_selector,
                   field_selector);

    execute format($resource_table_insert$
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
      if current_setting('omni_kube.refresh', true) = 'true' then
        return new;
      end if;
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
    $resource_table_insert$, table_name || '_insert', ns, resource_kind, group_version, url);

    execute format($resource_table_insert_t$
    create trigger %1$I before insert on %2$I for each row
    execute function %1$I()
    $resource_table_insert_t$, table_name || '_insert', table_name);

    execute format($resource_table_update$
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
      if current_setting('omni_kube.refresh', true) = 'true' then
        return new;
      end if;
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
      new.name := result->'metadata'->>'name';
      new.namespace := result->'metadata'->>'namespace';
      new.resource := result;
      return new;
    end;
    $$
    $resource_table_update$, table_name || '_update', ns, url);

    execute format($resource_table_update_t$
    create trigger %1$I before update on %2$I for each row
    execute function %1$I()
    $resource_table_update_t$, table_name || '_update', table_name);

    execute format($resource_table_delete$
    create or replace function %I() returns trigger
    set search_path to %I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name text;
      url text;
    begin
      if current_setting('omni_kube.refresh', true) = 'true' then
        return old;
      end if;
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
    $resource_table_delete$, table_name || '_delete', ns, url);

    execute format($resource_table_delete_t$
    create trigger %1$I before delete on %2$I for each row
    execute function %1$I()
    $resource_table_delete_t$, table_name || '_delete', table_name);

    return table_name;
end;
$resource_table$;
