-- This is an amount type, representing non-negative amounts,
-- temporarily implemented as a domain. TODO: make it an unsigned
-- (pending omni_int release)
create domain amount as bigint check (value >= 0);

select identity_type('ledger_id', type => 'uuid', nextval => 'uuidv7()');

create table ledgers
(
    id ledger_id primary key default ledger_id_nextval()
);

select pg_catalog.pg_extension_config_dump('ledgers', '');

select identity_type('account_id', type => 'uuid', nextval => 'uuidv7()');

create table accounts
(
    id                               account_id primary key default account_id_nextval(),
    ledger_id                        ledger_id not null references ledgers (id),
    debits_allowed_to_exceed_credits boolean   not null     default false,
    credits_allowed_to_exceed_debits boolean   not null     default false,
    closed                           boolean   not null     default false,
    check ( credits_allowed_to_exceed_debits or debits_allowed_to_exceed_credits)
);

select pg_catalog.pg_extension_config_dump('accounts', '');

create table account_balance_slots
(
    account_id account_id not null references accounts (id),
    debited    amount     not null default 0,
    credited   amount     not null default 0,
    slot       int        not null,
    constraint account_slot_unique unique (account_id, slot)
);

select pg_catalog.pg_extension_config_dump('account_balance_slots', '');

create view account_balances as
select account_id,
       sum(debited) as debited,
       sum(credited)           as credited,
       sum(credited - debited) as balance
from account_balance_slots
group by account_id;

select identity_type('transfer_id', type => 'uuid', nextval => 'uuidv7()');

create table transfers
(
    id                transfer_id primary key default transfer_id_nextval(),
    debit_account_id  account_id not null references accounts (id),
    credit_account_id account_id not null references accounts (id),
    amount amount not null
);

select pg_catalog.pg_extension_config_dump('transfers', '');

create function transaction_affected_accounts()
    returns table
            (
                account_id account_id,
                ledger_id  ledger_id
            )
    language c
as
'MODULE_PATHNAME';

create function statement_affected_accounts()
    returns table
            (
                account_id account_id,
                ledger_id  ledger_id
            )
    language c
as
'MODULE_PATHNAME';


/*{% include "../src/deny_operation_on_immutable_table.sql" %}*/

create function deny_operation_on_accounts() returns trigger
    language c as
'MODULE_PATHNAME';

create function calculate_account_balances() returns trigger
    language c as
'MODULE_PATHNAME' stable;

create trigger calculate_account_balances
    after insert
    on transfers
    for each row
execute function calculate_account_balances();

create function update_account_balances() returns trigger
    language c as
'MODULE_PATHNAME' stable;

create trigger update_account_balances
    after insert
    on transfers
    for each statement
execute function update_account_balances();

create trigger immutable_transfers
    before delete or update or truncate
    on transfers
    for each statement
execute function deny_operation_on_immutable_table();

create trigger immutable_accounts
    before delete or update
    on accounts
    for each row
execute function deny_operation_on_accounts();

create trigger untruncatable_accounts
    before truncate
    on transfers
    for each statement
execute function deny_operation_on_immutable_table();

/*{% include "../src/ensure_affected_accounts_balance.sql" %}*/

create trigger balancing_accounts
    after insert
    on transfers
    for each statement
execute function ensure_affected_accounts_balance();

comment on trigger balancing_accounts on transfers is
    $$ A debugging assertion consistency check that ensures that every transfer is balanced between debit and credit side.
    Disabled by default as users can't violate this invariant, but omni_ledger, in theory, can.$$;

alter table transfers
    disable trigger balancing_accounts;