# Account Categories

Built on top of the [core](ledger_core.md), account categories provide further assurance
that the accounts balance from the accounting rules perspective.

There are a few tables provided by omni_ledger to facilitate this.

## `omni_ledger.account_categories`

|           Column | Type                         | Description                                                    |
|-----------------:|------------------------------|----------------------------------------------------------------|
|           **id** | account_category_id (bigint) | Identifier of the category                                     | 
|         **name** | text                         | Name of the category (such as 'Assets' or 'Expenses')          |
|         **type** | category_type                | one of 'asset', 'liability' or 'equity'                        |
| **debit_normal** | boolean                      | Is this account debit normal [^debit-normal]? (default: false) |

[^debit-normal]: Accounts where a debit increases the balance, such as assets or expenses.

This table allows one to set up their own chart of accounts. The most important columns are
**type** and **debit_normal** as they are used in
the [accounting equation](https://en.wikipedia.org/wiki/Accounting_equation) verification that happens on every account
that is [categorized]().

This table enforces the following rules:

* Category can't be deleted unless there are no accounts categorized with it.

## `omni_ledger.account_categorizations`

|          Column | Type                | Description                                       |
|----------------:|---------------------|---------------------------------------------------|
|  **account_id** | account_id          | Identifier of the category the account belongs to | 
| **category_id** | account_category_id | Identifier of the category the account belongs to | 

Unless included in this table, [core accounts](ledger_core.md#accounts-omni_ledgeraccounts)
are not subject to accounting equation verification.

This table enforces a few rules:

* An account can only be categorized only with one category
* An account can only by categorized once in its lifetime
* Categorizations can't be deleted