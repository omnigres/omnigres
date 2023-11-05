create function import_mime_db(data jsonb) returns void
as
$$
declare
    item         record;
    mime_type_id_ integer;
begin
    for item in select (jsonb_each(data)).*
        loop
            -- Insert mime type
            insert
            into
                omni_mimetypes.mime_types (name, source, compressible, charset)
                (select
                     item.key,
                     (item.value ->> 'source')::omni_mimetypes.mime_type_source,
                     (item.value ->> 'compressible')::bool,
                     item.value ->> 'charset')
            on conflict (name) do update set
                                             source       = (item.value ->> 'source')::omni_mimetypes.mime_type_source,
                                             compressible = (item.value ->> 'compressible')::bool,
                                             charset      = item.value ->> 'compressible'
            returning id into mime_type_id_;
            -- Insert extensions
            if item.value ? 'extensions' then
                -- Account for potential disassociation of extensions
                delete from omni_mimetypes.mime_types_file_extensions where mime_type_id = mime_type_id_;
                -- Insert extensions and their associations
                raise notice '%s', (item.value->>'extensions');
                with
                    extensions as
                        (insert into omni_mimetypes.file_extensions (extension) (select jsonb_array_elements_text(item.value -> 'extensions'))
                            on conflict (extension) do update set extension = file_extensions.extension returning id)
                insert
                into
                    omni_mimetypes.mime_types_file_extensions (mime_type_id, file_extension_id)
                select
                    mime_type_id_,
                    id
                from
                    extensions;
            end if;
        end loop;
    return;
end;
$$ language plpgsql;

create function import_mime_db(data json) returns void
as $$
    select omni_mimetypes.import_mime_db(data::jsonb)
$$ language sql;