begin;
select omni_seq.prefix_seq_int64_int64_make(100, 1);
end;

begin;

select
    (omni_seq.system_identifier() = omni_seq.system_identifier() and omni_seq.system_identifier() is not null) as valid;

create sequence seq;
create table t
(
    id omni_seq.prefix_seq_int64_int64 primary key not null default
        omni_seq.prefix_seq_int64_int64_nextval(10, 'seq')
);

insert
into
    t
select
from
    generate_series(0, 10);
table t;
rollback;