# Ledger Basics

`omni_ledger` is an extension for building financial or finance-like systems where
transfers of value are recorded and balanced. It is designed to be minimalistic yet flexible.

### Ledger

A [ledger](ledger_core.md#ledgers-omni_ledgerledgers) in omni_ledger is a **set of accounts** that collectively
represent a specific financial entity, such as a
currency, a unit of value, or an internal accounting system for a company. Each ledger can track the balances and
transfers between its accounts independently. For example, a ledger might represent all accounts related to a single
currency like USD or EUR, or it could represent the internal funds within a specific department or entity in a company.

Ledgers are flexible, allowing you to define and group accounts according to your system's needs, whether for tracking
an external currency or managing internal units of value. Each ledger maintains its own balance sheet, where all debits
and credits across accounts must balance, ensuring the integrity of the financial data.

### Account

An [account](ledger_core.md#accounts-omni_ledgeraccounts) in omni_ledger represents a distinct entity within a ledger to
which value can be **credited** or **debited**.
Each account holds a [balance](ledger_core.md#account-balances-omni_ledgeraccount_balances) , which reflects the total
value it controls at any given moment. Accounts are central
to
the operation of a ledger, as they are the points between which value moves during transfers.

??? tip "Use cases for accounts"

    Accounts are highly flexible and can be used to represent a wide range of financial constructs, such as:

    * A customer's wallet in a digital payment system
    * A company's bank account
    * An internal fund allocated for a specific project
    * A liability account representing a loan or debt

    ??? example "Enforcement of business rules"
    
        Even more so, because accounts are very lightweight, omni_ledger users are encouraged to use them to represent scoped activities such as:

        * Capturing partial bill payments
        * Transactions that go through settlement processes

        By doing this, business rules specific to these processes can be directly attached to the accounts, allowing to enforce
        necessary business constraints right within the ledger. This ensures that the ledger not only tracks value but also upholds
        the operational rules that govern how transactions are handled.

    In summary, accounts are fundamental to the operation of the ledger, as they serve as the entities between which value
    flows. Their balances, ownership, and types define their role in the system, allowing omni_ledger to support diverse
    financial and operational model

### Transfer

A [transfer](ledger_core.md#transfers-omni_ledgertransfers) represents the **movement of value** between two accounts.
In omni_ledger, a transfer is an atomic operation, meaning that it always results in balanced debits and credits across
accounts. Transfers typically correspond to specific events or transactions, such as payments, refunds, or fund
allocations. Each transfer is recorded in the ledger and forms the basic unit of value movement in the system.

!!! question "Why is it not called a transaction?"

     To avoid confusion between financial and database transactions, we refer to the act of debiting and crediting accounts
     as transfers in omni_ledger.

### Account Category

An [account category](account_categories.md) classifies accounts based on their financial nature within the system.
Categories help structure the ledger by grouping accounts under standard financial headings, such as **assets**, *
*liabilities**, or **equity**. These categories ensure that each account adheres to accounting rules for that type. In
omni_ledger, once an account is assigned to a category, it remains associated with that category, providing clarity and
consistency.

