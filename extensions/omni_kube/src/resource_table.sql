create function resource_table(table_name name, group_version text, resource text, label_selector text default null,
                               field_selector text default null, transactional boolean default false)
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
    url := resources_path(group_version, resource, namespace => case when is_namespaced then '%s' end);

    perform
        set_config('search_path', old_search_path, true);


    execute format($resource_table_$
    create table %I as
    select
     resource->'metadata'->>'uid' as uid,
     resource->'metadata'->>'name' as name,
     resource->'metadata'->>'namespace' as namespace,
     * from %I.resources(%L, %L, label_selector => %L, field_selector => %L) resource
    $resource_table_$, table_name, ns, group_version, resource, label_selector, field_selector);

    if transactional then
        execute format('alter table %1$I add column new boolean not null default false', table_name);
        execute format('alter table %1$I add column committed boolean not null default true', table_name);
        execute format($resource_queue_table_$
            create table %1$I (
               name text,
               namespace text,
               url text,
               body jsonb,
               method omni_http.http_method,
               unique (name, namespace)
            );
            create or replace function %1$I()
            returns trigger language plpgsql
            as $$
            declare
             rec record;
            begin
              perform from %1$I;
              if not found then
                return new;
              end if;
              with i as (delete from %1$I returning url, method, body)
                 select %3$I.api(array_agg(url), methods => array_agg(method), bodies => array_agg(body)) from i into rec;
              perform %4$I();
              return new;
            end;
            $$;

            create constraint trigger %1$I
                after insert or update or delete
                on %2$I
                deferrable initially deferred
                for each row
            execute function %1$I();
            $resource_queue_table_$, 'omni_kube<' || table_name || '>', table_name, ns, 'refresh_' || table_name);

    end if;


    execute format($$create unique index %2$I on %1$I (uid) $$, table_name, table_name || '_index_uid');
    execute format(
            $$create unique index %2$I on %1$I (name, namespace) $$,
            table_name,
            table_name || '_index_name_space');

    execute format($table_resource_$
    create function %1$I() returns text return %2$I.path_with(%2$I.resources_path(%3$L, %4$L), label_selector => %5$L, field_selector => %6$L)
    $table_resource_$, table_name || '_resource_path', ns, group_version, resource, label_selector,
                   field_selector);


    execute format($refresh_agg$
    create function %3$I(jsonb) returns jsonb
    set omni_kube.refresh = true
    begin atomic;
    with new as materialized (select
     resource->'metadata'->>'uid' as uid,
     resource->'metadata'->>'name' as name,
     resource->'metadata'->>'namespace' as namespace,
     resource from jsonb_array_elements($1) resource),
       deletion as (delete from %2$I where not exists (select from new where new.uid = %2$I.uid) returning %2$I.*),
       insertion as (insert into %2$I select * from new where not exists (select from %2$I x where x.uid = new.uid) returning %2$I.*),
       updating as (update %2$I set uid = new.uid, name = new.name, namespace = new.namespace, resource = new.resource
          from new where new.uid = %2$I.uid and new.resource != %2$I.resource returning %2$I.*)
       select coalesce(jsonb_agg(to_jsonb(t)),'[]') from (
       select 'DELETED' as type, od.resource as object from deletion od
       union all
       select 'ADDED' as type, oi.resource as object from insertion oi
       union all
       select 'MODIFIED' as type, ou.resource as object from updating ou) t;
    end;
    create aggregate %1$I (jsonb) (sfunc = jsonb_concat, finalfunc = %3$I, stype = jsonb, initcond = '[]');
    $refresh_agg$, 'refresh_' || table_name || '_agg', table_name, 'refresh_' || table_name || '_agg_sfunc');

    execute format($resource_table_refresh$
    create function %1$I() returns table (type text, object jsonb)
    begin atomic; with updates as (select jsonb_array_elements(%2$I(api->'items')) r from %4$I.api(%3$I())) select r->>'type', r->'object' from updates; end;
    $resource_table_refresh$, 'refresh_' || table_name, 'refresh_' || table_name || '_agg',
                   table_name || '_resource_path', ns);


    execute format($resource_table_insert$
    create or replace function %1$I() returns trigger
    set search_path to %2$I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name_ text;
      canonical_url text;
      url text;
      metadata jsonb;
      result jsonb;
    begin
      if current_setting('omni_kube.refresh', true) = 'true' then
        if %6$L::boolean then
          delete from %7$I where name = new.name and namespace = new.namespace;
        end if;
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
      name_ := coalesce(coalesce(metadata->>'name', metadata->>'generateName'), new.name);
      if name_ is null then
        raise exception 'resource is required to have `name` or `generateName`';
      end if;
      if new.name is not null and name_ != new.name then
        raise exception 'name (%%) must match metadata (%%)', new.name, name_;
      end if;
      new.resource := jsonb_set(new.resource, '{kind}', to_jsonb(%3$L::text));
      new.resource := jsonb_set(new.resource, '{apiVersion}', to_jsonb(%4$L::text));
      canonical_url := format(%5$L, spec_ns);
      url := canonical_url;
      -- transactional?
      if %6$L::boolean and not current_setting('omni_kube.refresh', true) = 'true' then
        url := path_with(canonical_url, dry_run => true);
        new.committed = false;
        new.new := true;
      end if;
      select api(url, body => new.resource, method => 'POST') into result;
      new.uid := result->'metadata'->>'uid';
      new.name := result->'metadata'->>'name';
      new.namespace := result->'metadata'->>'namespace';
      new.resource := result;
      if %6$L::boolean and not current_setting('omni_kube.refresh', true) = 'true' then
        insert into %7$I as t (name,namespace,url,method,body) values (new.name,new.namespace, canonical_url,'POST',new.resource)
        on conflict (name, namespace) do update set name = new.name, namespace = new.namespace, url = canonical_url, method = 'POST', body = new.resource;
      end if;
      return new;
    end;
    $$
    $resource_table_insert$, table_name || '_insert', ns, resource_kind, group_version, url, transactional,
                   'omni_kube<' || table_name || '>');

    execute format($resource_table_insert_t$
    create trigger %1$I before insert on %2$I for each row
    execute function %1$I()
    $resource_table_insert_t$, table_name || '_insert', table_name);

    execute format($resource_table_update$
    create or replace function %1$I() returns trigger
    set search_path to %2$I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name_ text;
      url text;
      canonical_url text;
      result jsonb;
      new_ boolean := false;
      method_ omni_http.http_method := 'PUT';
    begin
      if current_setting('omni_kube.refresh', true) = 'true' then
        if %4$L::boolean then
          delete from %5$I where name = new.name and namespace = new.namespace;
        end if;
        return new;
      end if;
      if %4$L::boolean then
       new_ := new.new;
       if new_ then
         method_ := 'POST';
       end if;
      end if;
      spec_ns := coalesce(coalesce(new.resource->'metadata','{"namespace": "default"}'::jsonb)->>'namespace', 'default');
      if new.namespace is not null and spec_ns != new.namespace then
        raise exception 'namespace (%%) must match metadata (%%)', new.namespace, spec_ns;
      end if;
      name_ := coalesce(coalesce(new.resource->'metadata','{}')->>'name', new.name);
      if name_ is null then
        raise exception 'resource is required to have a name';
      end if;
      if new.name is not null and name_ != new.name then
        raise exception 'name (%%) must match metadata (%%)', new.name, name;
      end if;
      canonical_url := format(%3$L, spec_ns);
      if not new_ then
        canonical_url := canonical_url || '/' || new.name;
      end if;
      url := canonical_url;
      if %4$L::boolean then
        url := path_with(canonical_url, dry_run => true);
        new.committed := false;
      end if;
      select api(url, body => new.resource, method => method_) into result;
      new.name := result->'metadata'->>'name';
      new.namespace := result->'metadata'->>'namespace';
      new.resource := result;
      if %4$L::boolean then
        insert into %5$I as t (name,namespace,url,method,body) values (new.name,new.namespace, canonical_url,method_,new.resource)
        on conflict (name, namespace) do update set name = new.name, namespace = new.namespace, url = canonical_url, method = method_, body = new.resource;
      end if;
      return new;
    end;
    $$
    $resource_table_update$, table_name || '_update', ns, url, transactional, 'omni_kube<' || table_name || '>');

    execute format($resource_table_update_t$
    create trigger %1$I before update on %2$I for each row
    execute function %1$I()
    $resource_table_update_t$, table_name || '_update', table_name);

    execute format($resource_table_delete$
    create or replace function %1$I() returns trigger
    set search_path to %2$I, public
    language plpgsql as
    $$
    declare
      spec_ns text;
      name_ text;
      canonical_url text;
    begin
      if current_setting('omni_kube.refresh', true) = 'true' then
        if %4$L::boolean then
          delete from %5$I where name = old.name and namespace = old.namespace;
        end if;
        return old;
      end if;
      spec_ns := coalesce(coalesce(old.resource->'metadata','{"namespace": "default"}'::jsonb)->>'namespace', 'default');
      if old.namespace is not null and spec_ns != old.namespace then
        raise exception 'namespace (%%) must match metadata (%%)', old.namespace, spec_ns;
      end if;
      name_ := coalesce(coalesce(old.resource->'metadata','{}')->>'name', old.name);
      if name_ is null then
        raise exception 'resource is required to have a name';
      end if;
      if old.name is not null and name_ != old.name then
        raise exception 'name (%%) must match metadata (%%)', old.name, name_;
      end if;
      canonical_url := format(%3$L, spec_ns);
      if not %4$L::boolean then
        perform api(canonical_url || '/' || name_, method => 'DELETE');
      else
        if not old.new then
          insert into %5$I as t (name,namespace,url,method,body) values (old.name,old.namespace, canonical_url,'DELETE','{}')
          on conflict (name, namespace) do update set name = old.name, namespace = old.namespace, url = canonical_url, method = 'DELETE', body = null;
        else
          delete from %5$I where name = old.name and namespace = old.namespace;
        end if;
      end if;
      return old;
    end;
    $$
    $resource_table_delete$, table_name || '_delete', ns, url, transactional, 'omni_kube<' || table_name || '>');

    execute format($resource_table_delete_t$
    create trigger %1$I before delete on %2$I for each row
    execute function %1$I()
    $resource_table_delete_t$, table_name || '_delete', table_name);

    return table_name;
end;
$resource_table$;
