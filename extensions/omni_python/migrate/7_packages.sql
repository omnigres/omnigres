create function installed_packages() returns table (name text, version text)
    language plpython3u
as
$$
/*{% include "../src/installed_packages.py" %}*/
$$;

create view installed_package as select * from installed_packages();
