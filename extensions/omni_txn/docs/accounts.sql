create table accounts
(
    id        serial primary key,
    closed_on timestamptz
);

create table account_transfers
(
    time              timestamptz primary key default statement_timestamp(),
    debit_account_id  int references accounts (id),
    credit_account_id int references accounts (id),
    amount            bigint
);