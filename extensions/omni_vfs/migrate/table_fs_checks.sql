-- Checks

do
$$
    begin
        if not omni_vfs_types_v1.is_valid_fs('local_fs') then
            raise exception 'local_fs is not a valid vfs';
        end if;
    end;
$$ language plpgsql;