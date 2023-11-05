create function static_file_handlers(fs regproc, handler_priority int, listing bool default false)
    returns table
            (
                name     text,
                query    text,
                priority int
            )
    language plpgsql
as
$plpgsql$
begin
    perform from pg_extension where extname = 'omni_vfs';
    if not found then
        raise exception 'omni_vfs required';
    end if;
    perform from pg_extension where extname = 'omni_mimetypes';
    if not found then
        raise exception 'omni_mimetypes required';
    end if;
    perform
    from
        pg_proc
    where
            oid = fs::oid and
            (array_length(proargnames, 1) = 0 or proargnames is null) and
            omni_vfs_types_v1.is_valid_fs(prorettype);
    if not found then
        raise exception 'must have %() function with no arguments returning a valid omni_vfs filesystem', fs;
    end if;
    return query
        select
            handler_name,
            handler_query,
            handler_priority
        from
            (values
                 ('file',
                  format($$select omni_httpd.http_response(
            omni_vfs.read(%1$s(), request.path),
            headers => array [omni_http.http_header('content-type', coalesce (mime_types.name, 'application/octet-stream'))]::omni_http.http_header[])
            from request
            left join omni_mimetypes.file_extensions on request.path like '%%.' || file_extensions.extension
            left join omni_mimetypes.mime_types_file_extensions mtfe on mtfe.file_extension_id = file_extensions.id
            left join omni_mimetypes.mime_types on mtfe.mime_type_id = mime_types.id
            where request.method = 'GET' and (omni_vfs.file_info(%1$s(), request.path)).kind = 'file'
        $$, fs)),
                 ('directory',
                  format($$
        select
            omni_httpd.http_response(
                    omni_vfs.read(%1$s(), request.path || '/index.html'),
                    headers => array [omni_http.http_header('content-type', 'text/html')]::omni_http.http_header[])
        from
            request
        where
            request.method = 'GET' and
            (omni_vfs.file_info(%1$s(), request.path)).kind = 'dir' and
            (omni_vfs.file_info(%1$s(), request.path || '/index.html')).kind = 'file'
            $$, fs)),
                 ('directory_listing',
                  format($$
        select
        omni_httpd.http_response(
           (select string_agg('<a href="' || case when request.path = '/' then '/' else request.path || '/' end  || name || '">' || name || '</a>', '<br>') from omni_vfs.list(%1$s(), request.path)),
           headers =>  array [omni_http.http_header('content-type', 'text/html')]::omni_http.http_header[])
        from
            request
        where
            request.method = 'GET' and
            (omni_vfs.file_info(%1$s(), request.path)).kind = 'dir' and
            (omni_vfs.file_info(%1$s(), request.path || '/index.html')) is not distinct from null
            $$, fs))) handlers(handler_name, handler_query)
        where
            (handler_name = 'directory_listing' and listing) or
            (handler_name != 'directory_listing');
end;
$plpgsql$;