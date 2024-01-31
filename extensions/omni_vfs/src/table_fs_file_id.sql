create function table_fs_file_id(fs table_fs, path text) returns bigint
    stable
    language sql set search_path to omni_vfs
as
$$
with
    query as (with
                  components as (select *
                                 from
                                     regexp_split_to_table(
                                             case
                                                 when omni_vfs.canonicalize_path(path, absolute => true) = '/' then ''
                                                 else omni_vfs.canonicalize_path(path, absolute => true)
                                                 end,
                                             '/'
                                         )
                                         with ordinality x(component, depth)),
                  depth as (select max(depth) as max_depth
                            from components)
              select *
              from
                  components,
                  depth),
    match as (with
                  recursive
                  r as (select
                            f.depth,
                            q.max_depth,
                            f.id,
                            f.filename,
                            f.kind
                        from
                            table_fs_files   f
                            inner join query q on f.filesystem_id = fs.id and f.depth = 1 and q.depth = 1 and
                                                  q.component = f.filename
                        union
                        select
                            f.depth,
                            q.max_depth,
                            f.id,
                            f.filename,
                            f.kind
                        from
                            r
                            inner join table_fs_files f on coalesce(f.parent_id, 0) = r.id
                            inner join query          q on q.depth = r.depth + 1 and q.component = f.filename)
              select
                  id
              from
                  r
              where
                  r.depth = r.max_depth)
select
        (select * from match);
$$;