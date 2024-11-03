create function to_json(text) returns json
    strict
    language c as
'MODULE_PATHNAME',
'yaml_to_json';

create function to_yaml(json) returns text
    strict
    language c as
'MODULE_PATHNAME';