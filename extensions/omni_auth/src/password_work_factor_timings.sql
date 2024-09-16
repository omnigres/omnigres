create function password_work_factor_timings(iterations int default 3, timeout_ms double precision default 1500)
    returns table
            (
                algorithm       hashing_algorithm,
                work_factor     int,
                iteration       int,
                time_elapsed_ms double precision
            )
    language plpgsql
    parallel safe
as
$$
declare
    current_work_factor int;
    current_iteration   int;
    start_time          timestamp;
    end_time            timestamp;
begin
    -- bcrypt
    algorithm := 'bcrypt';

    for current_work_factor in 10..31
        loop
            for current_iteration in 1..iterations
                loop
                    work_factor := current_work_factor;
                    iteration := current_iteration;
                    start_time := clock_timestamp();
                    perform omni_auth.hash_password('password', hashing_algorithm => algorithm,
                                                    work_factor => current_work_factor);
                    end_time := clock_timestamp();
                    time_elapsed_ms := (extract(epoch from end_time) - extract(epoch from start_time)) * 1000;
                    if time_elapsed_ms >= timeout_ms then
                        return;
                    end if;
                    return next;
                end loop;
        end loop;
    return;

end;
$$;