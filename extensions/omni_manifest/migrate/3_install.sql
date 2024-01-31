create type requirement_status as enum ('installed', 'missing', 'updated');

create type install_report as
(
    requirement requirement,
    status      requirement_status
);
/*{% include "../src/install.sql" %}*/
