create or replace function jsonb_set_at_path(target jsonb, path text[], new_value jsonb)
    returns jsonb as
$$
declare
    i            int;
    current_path text[];
    temp         jsonb;
    is_numeric   bool;
begin
    temp := target;

    for i in 1..array_length(path, 1)
        loop
            current_path := path[1:i];

            -- Check if the current path segment is numeric
            is_numeric := path[i] ~ E'^\\d+$';

            -- If the current path doesn't exist in temp
            if (temp #> current_path) is null then
                -- If the segment is numeric
                if is_numeric then
                    if ((temp #> current_path[1:i - 1]) = '{}'::jsonb) then
                        temp := jsonb_set(temp, current_path[1:i - 1], '[]');
                    end if;

                    -- If it's an array index, ensure the array has enough elements
                    if jsonb_typeof(temp #> current_path[1:i - 1]) = 'array' then
                        while jsonb_array_length(temp #> current_path[1:i - 1]) < path[i]::int
                            loop
                                temp := jsonb_set(temp, current_path[1:i - 1],
                                                  (temp #> current_path[1:i - 1]) || 'null'::jsonb);
                            end loop;
                        -- Set the value at the specific index
                        temp := jsonb_insert(temp, current_path, new_value);
                    end if;

                else
                    -- If it's not a numeric segment, ensure the path and then set the value
                    if i = array_length(path, 1) then
                        temp := jsonb_set(temp, current_path[1:i], new_value);
                    else
                        temp := jsonb_insert(temp, current_path[1:i], '{}'::jsonb);
                    end if;
                end if;
            else
                if is_numeric then
                    temp := jsonb_set(temp, current_path, new_value);
                end if;
            end if;
        end loop;

    return temp;
end;
$$ language plpgsql;