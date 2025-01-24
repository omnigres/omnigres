create domain password varchar(255);

create type hashing_algorithm as enum ('bcrypt');

create domain hashed_password text check (
    -- bcrypt
    value ~ '^\$2[aby]\$[0-9]{2}\$[./A-Za-z0-9]{53}$'
    );

/*{% include "../src/work_factor.sql" %}*/
/*{% include "../src/hashing_algorithm.sql" %}*/

/*{% include "../src/hash_password.sql" %}*/

select identity_type('password_credential_id', type => 'uuid', nextval => 'uuidv7()');

create table password_credentials
(
    id password_credential_id not null default password_credential_id_nextval(),
    authentication_subject_id authentication_subject_id not null references authentication_subjects (id),
    hashed_password hashed_password not null,
    valid_at        tstzrange not null default tstzrange(statement_timestamp(), 'infinity'),
    constraint password_credentials_valid_at_no_overlap exclude using gist ((authentication_subject_id::uuid) with =, valid_at with &&)
);

/*{% include "../src/set_password.sql" %}*/

comment on table password_credentials is $$
Password credentials are temporally-constrained hashed passwords for authentication subjects
$$;

select pg_catalog.pg_extension_config_dump('password_credentials', '');

select identity_type('password_authentication_attempt_id', type => 'uuid', nextval => 'uuidv7()');

--- We store password authentication attempts for detecting suspicious behavior
--- Example: HIPAA § 164.308(a)(5)(ii)(C)

create table password_authentication_attempts
(
    id                        password_authentication_attempt_id not null primary key default password_authentication_attempt_id_nextval(),
    authentication_subject_id authentication_subject_id references authentication_subjects (id),
    hashed_password        hashed_password,
    success     boolean   not null,
    occurred_at timestamp not null default statement_timestamp()
);

select pg_catalog.pg_extension_config_dump('password_authentication_attempts', '');

/*{% include "../src/authenticate_password.sql" %}*/

/*{% include "../src/successful_authentification_password_authentication_attempts.sql" %}*/

-- Informational section

--- To determine appropriate work factor, it would be useful to have some information on how long does it take to hash
--- for any given algorithm and work factor. OWASP suggestion is to target under 1s for hashing to ensure decent user experience
--- (https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#using-work-factors)

/*{% include "../src/password_work_factor_timings.sql" %}*/

create materialized view password_work_factor_timings as
select *
from password_work_factor_timings() with no data;

create unique index password_work_factor_timings_idx on password_work_factor_timings (algorithm, work_factor, iteration);

