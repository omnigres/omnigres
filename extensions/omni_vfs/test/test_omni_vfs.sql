\set ON_ERROR_STOP on

-- Test for root directory in table_fs
do $$ 
begin
    perform file_info('table_fs', '/');
    assert found, 'Expected non-null result for root directory in table_fs';
end $$;