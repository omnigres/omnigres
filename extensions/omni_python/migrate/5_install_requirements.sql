create function install_requirements(requirements text) returns void
    language plpython3u
as
$$
/*{% include "../src/install_requirements.py" %}*/
$$;
