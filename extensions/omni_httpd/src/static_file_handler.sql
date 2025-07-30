create
    or
    replace function static_file_handler(request omni_httpd.http_request,
                                         fs anyelement, fs_path text default '', path text default null,
                                         listing boolean default false)
    returns omni_httpd.http_outcome
    language plpgsql as
$static_file_handler$
declare
    outcome       omni_httpd.http_outcome;
    file_content  bytea;
    etag          text;
    if_none_match text;
    mime_type     text;
begin
    if path is null then
        path := request.path;
    end if;

    --- Directory
    if request.method = 'GET' and
       ((path = '/') or
        (omni_vfs.file_info(fs, fs_path || path)).kind = 'dir') and
       (omni_vfs.file_info(fs, fs_path || path || '/index.html')).kind = 'file' then
        path := path || '/index.html';
    end if;

    --- File
    if request.method = 'GET' and
       (omni_vfs.file_info(fs, fs_path || path)).kind = 'file' then
        file_content := omni_vfs.read(fs, fs_path || path);
        etag := '"' || encode(digest(file_content, 'sha256'), 'hex') || '"';
        if_none_match := omni_http.http_header_get(request.headers, 'if-none-match');

        select
            coalesce(mime_types.name, 'application/octet-stream')
        into mime_type
        from
                (select)                                            _
                left join omni_mimetypes.file_extensions on path like '%%.' || file_extensions.extension
                left join omni_mimetypes.mime_types_file_extensions mtfe
                          on mtfe.file_extension_id = file_extensions.id
                left join omni_mimetypes.mime_types on mtfe.mime_type_id = mime_types.id;

        if if_none_match = etag then
            -- Return 304 Not Modified
            select
                omni_httpd.http_response(
                        status => 304,
                        headers => array [
                            omni_http.http_header('content-type', mime_type),
                            omni_http.http_header('etag', etag),
                            omni_http.http_header('cache-control', 'public, max-age=3600')
                            ]
                )
            into outcome;
        else
            select
                omni_httpd.http_response(
                        file_content,
                        headers => array [omni_http.http_header('content-type',
                                                                mime_type),
                            omni_http.http_header('etag', etag),
                            omni_http.http_header('cache-control', 'public, max-age=3600')
                            ])
            into outcome;
        end if;
    end if;

    if outcome is not null then
        return outcome;
    end if;

    --- File listing

    if listing and request.method = 'GET' and
       ((path = '/') or
        (omni_vfs.file_info(fs, fs_path || path)).kind = 'dir') and
       (omni_vfs.file_info(fs, fs_path || path || '/index.html')) is not distinct from null then
        select
            omni_httpd.http_response(
                    (select
                         string_agg('<a href="' || case when path = '/' then '/' else path || '/' end ||
                                    name ||
                                    '">' || name || '</a>', '<br>')
                     from
                         omni_vfs.list(fs, fs_path || path)),
                    headers => array [omni_http.http_header('content-type', 'text/html')]::omni_http.http_header[])

        into outcome;
    end if;

    if outcome is not null then
        return outcome;
    end if;

    return omni_httpd.http_response(status => 404);
end;
$static_file_handler$;
