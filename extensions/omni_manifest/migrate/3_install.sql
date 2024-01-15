create type artifact_status as enum ('installed', 'missing', 'updated');

create type install_report as
(
    requirement requirement,
    status      artifact_status
);
/*{% include "../src/install.sql" %}*/
