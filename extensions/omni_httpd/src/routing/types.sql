-- Types needed for routing functionality
CREATE TYPE router_config AS (
    router_name name,
    router_type name,
    route_regexp text,
    method text,
    host_regexp text
);