-- Checks

do
$$
    begin
        if not omni_vfs_types_v1.is_valid_fs('remote_fs') then
            raise exception 'remote_fs is not a valid vfs';
        end if;
    end;
$$ language plpgsql;