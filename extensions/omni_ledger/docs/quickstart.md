# Quickstart

In order to set up a minimally viable ledgering system, one must:

## Set up a ledger

```postgresql
insert into omni_ledger.ledgers
values (default)
returning id -- (1) note the ID
```

## Set up accounts

```postgresql
insert into omni_ledger.accounts (ledger_id, debits_allowed_to_exceed_credits, credits_allowed_to_exceed_debits)
values (noted_ledger_id, false, true), ...
    returning id -- (1) note their IDs
```

## Start transferring

It is important to ensure you are using serializable transactions:

```postgresql
begin transaction isolation level serializable;
insert into omni_ledger.transfers (debit_account_id, credit_account_id, amount)
values
...;
commit;
```

Since serializable transactions can fail due to serialization issues, you're advised to
use [omni_txn.retry](/omni_txn/retry)

## Set up accounting

Once ready, set up some basic accounts:

```postgresql
insert into omni_ledger.account_categories (name, type, debit_normal)
values ('Assets', 'asset', true),
       ('Owner''s equity', 'equity', false),
       ('Expenses', 'equity', true),
       ('Liabilities', 'liability', false);
```