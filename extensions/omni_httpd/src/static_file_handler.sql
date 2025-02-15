create
    or
    replace function static_file_handler(request omni_httpd.http_request,
                                         fs anyelement, path text default '', listing boolean default false)
    returns omni_httpd.http_outcome
    language plpgsql as
$static_file_handler$
declare
    outcome omni_httpd.http_outcome;
begin
    --- File
    if request.method = 'GET' and
       (omni_vfs.file_info(fs, path || request.path)).kind = 'file' then
        select
            omni_httpd.http_response(
                    omni_vfs.read(fs, path || request.path),
                    headers => array [omni_http.http_header('content-type',
                                                            coalesce(mime_types.name, 'application/octet-stream'))]::omni_http.http_header[])
        from
                (select)                                            _
                left join omni_mimetypes.file_extensions on request.path like '%%.' || file_extensions.extension
                left join omni_mimetypes.mime_types_file_extensions mtfe on mtfe.file_extension_id = file_extensions.id
                left join omni_mimetypes.mime_types on mtfe.mime_type_id = mime_types.id
        into outcome;
    end if;

    if outcome is not null then
        return outcome;
    end if;

    --- Directory
    if request.method = 'GET' and
       ((request.path = '/') or
        (omni_vfs.file_info(fs, path || request.path)).kind = 'dir') and
       (omni_vfs.file_info(fs, path || request.path || '/index.html')).kind = 'file' then
        select
            omni_httpd.http_response(
                    omni_vfs.read(fs, path || request.path || '/index.html'),
                    headers => array [omni_http.http_header('content-type', 'text/html')]::omni_http.http_header[])
        into outcome;
    end if;

    if outcome is not null then
        return outcome;
    end if;

    --- File listing

    if listing and request.method = 'GET' and
       ((request.path = '/') or
        (omni_vfs.file_info(fs, path || request.path)).kind = 'dir') and
       (omni_vfs.file_info(fs, path || request.path || '/index.html')) is not distinct from null then
        select
            omni_httpd.http_response(
                    (select
                         string_agg('<a href="' || case when request.path = '/' then '/' else request.path || '/' end ||
                                    name ||
                                    '">' || name || '</a>', '<br>')
                     from
                         omni_vfs.list(fs, path || request.path)),
                    headers => array [omni_http.http_header('content-type', 'text/html')]::omni_http.http_header[])

        into outcome;
    end if;

    if outcome is not null then
        return outcome;
    end if;

    return omni_httpd.http_response(status => 404);
end;
$static_file_handler$;