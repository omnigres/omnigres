-- Wait for the number of configuration reloads to be `n` or greater
-- Useful for testing
create procedure wait_for_configuration_reloads(n int) as
$$
declare
    c  int = 0;
    n_ int = n;
begin
    loop
        with
            reloads as (select
                            id
                        from
                            omni_httpd.configuration_reloads
                        order by happened_at asc
                        limit n_)
        delete
        from
            omni_httpd.configuration_reloads
        where
            id in (select id from reloads);
        declare
            rowc int;
        begin
            get diagnostics rowc = row_count;
            n_ = n_ - rowc;
            c = c + rowc;
        end;
        exit when c >= n;
    end loop;
end;
$$ language plpgsql;

