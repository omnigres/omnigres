create type category_type as enum ('asset', 'liability', 'equity');
select identity_type('account_category_id');

create table account_categories
(
    id                 account_category_id not null primary key default account_category_id_nextval(),
    name               text                not null,
    type               category_type       not null,
    debit_normal boolean not null default false
);

select pg_catalog.pg_extension_config_dump('account_categories', '');

/*{% include "../src/account_categories_immutability.sql" %}*/

create trigger account_categories_immutability
    before update
    on account_categories
    for each row
execute function account_categories_immutability();

create table account_categorizations
(
    account_id  account_id          not null references accounts (id),
    category_id account_category_id not null references account_categories (id) on delete restrict,
    unique (account_id, category_id)
);

select pg_catalog.pg_extension_config_dump('account_categorizations', '');

create trigger immutable_account_categorizations
    before delete or update or truncate
    on account_categorizations
    for each statement
execute function deny_operation_on_immutable_table();

create view accounting_balances as
select account_categorizations.account_id,
       account_categories.type    as category_type,
       credited,
       debited,
       balance * (case
                      when account_categories.debit_normal then -1
                      else 1 end) as balance
from account_categorizations
         inner join account_balances on account_balances.account_id = account_categorizations.account_id
         inner join account_categories on account_categories.id = account_categorizations.category_id;

/*{% include "../src/ensure_affected_accounts_accounting_balances.sql" %}*/

create trigger balancing_accounting
    after insert
    on transfers
    for each statement
execute function ensure_affected_accounts_accounting_balances();
