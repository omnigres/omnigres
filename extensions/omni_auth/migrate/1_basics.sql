select identity_type('authentication_subject_id', type => 'uuid', nextval => 'uuidv7');

create table authentication_subjects
(
    id authentication_subject_id not null primary key default authentication_subject_id_nextval()
);

select pg_catalog.pg_extension_config_dump('authentication_subjects', '');

comment on table authentication_subjects is $$
Authentication subject is a provisional term for subject of authentication, such as user.

Note, however, that we treat it a bit more broadly. For example, an unrecognized identifier (such as login
or e-mail) can also be an authentication subject. This way we can track attempts to authenticate against
a non-existent user.

This can also be useful in the context where we are doing an authentication for a user that does not yet exist,
especially in the context of third-party OAuth authentications-as-signups.
$$;