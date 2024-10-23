-- Types needed for routing functionality
create type router_config as (
    router_name name,
    router_type name,
    route_regexp text,
    method text,
    host_regexp text
);