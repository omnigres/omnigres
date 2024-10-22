# Ledger Core

All tables below use UUID-typed identifiers, generated as [UUIDv7](https://uuid7.com/) by
default.

## Ledgers (`omni_ledger.ledgers`)

Since every system attaches its own meaning to ledgers, the table only contains a unique identifier (**`id`**) as a
column. Additional information or attributes should be attached through referencing tables.

## Accounts (`omni_ledger.accounts`)

|                               Column | Type       | Description                                            |
|-------------------------------------:|------------|--------------------------------------------------------|
|                               **id** | account_id | Account's identifier (primary key)                     |
|                        **ledger_id** | ledger_id  | Reference to the ledger the account belongs to         |
| **debits_allowed_to_exceed_credits** | boolean    | Are debits allowed to exceed credits? (default: false) |
| **credits_allowed_to_exceed_debits** | boolean    | Are credits allowed to exceed debits? (default: false) |
|                           **closed** | boolean    | Is the account closed? (default: false)                |

Accounts provide enforcement of basic rules:

* If debits are not allowed to exceed credits, it means the account can't be debited for
  more than it has been credited for.
* If credits are not allowed to exceed debits, it means the account can't be credited for
  more than it was debited for.
* The account must enable at least one of the above allowances to be valid.
* Accounts can not be deleted.
* Once created, account properties cannot be modified, with the exception of setting **closed** to **true**.
* **closed** can't be set to **true** unless it has a zero balance and unless running in
  a _serializable_ transaction.
* No transfers can occur in closed accounts.

These rules provide mechanics for building more sophisticated cases.

### Business rules on accounts

To enforce additional rules on activities happening in the accounts, omni_ledger
provides two similar functions:

* `omni_ledger.statement_affected_accounts()`
* `omni_ledger.transaction_affected_accounts()`

Both return a table of `(account_id, ledger_id)` for accounts transacted on during the last statement and the current
transaction, respectively. These functions can be used in custom triggers to validate affected accounts.

## Transfers (`omni_ledger.transfers`)

|                Column | Type                      | Description                                     |
|----------------------:|---------------------------|-------------------------------------------------|
|                **id** | transfer_id               | Transfer's identifier (primary key)             |
|  **debit_account_id** | account_id                | Reference to the account that is being debited  |
| **credit_account_id** | account_id                | Reference to the account that is being credited |
|            **amount** | amount (bigint [^bigint]) | Non-negative amount                             |

This table enforces the following rules:

* Every transfer must be recorded in a _serializable_ transaction
* Closed accounts can't be debited or credited
* Amount must be non-negative

[^bigint]: The choice of the underlying type will be reassessed in the upcoming release to support a wider range of
values.

??? tip "Assurance of balanced accounts"

    `omni_ledger` offers an additional check to safeguard against
    potential imbalances in accounts caused by transfers due to a bug
    in the implementation.

    To ensure every transfer maintains account balance integrity,
    you can enable this the `balancing_accounts` trigger:

    ```postgresql
    alter table omni_ledger.transfers enable trigger balancing_accounts
    ```

    Please note that it is not strictly necessary to do so and it may have
    performance implications. This is more of a higher-assurance assertion check.

## Account balances (`omni_ledger.account_balances`)

|         Column | Type       | Description                            |
|---------------:|------------|----------------------------------------|
| **account_id** | account_id | Reference to an account                |
|    **debited** | numeric    | Total amount debited from this account |
|   **credited** | numeric    | Total amount credited to this account  |
|    **balance** | numeric    | Balance of the account                 |

This _view_ provides an insight into how much an account has been debited and credited for
as well as the balance of those operations.