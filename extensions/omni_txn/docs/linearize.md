# Transaction Linearization

For most common cases, transaction isolation levels like [
`serializable`](https://www.postgresql.org/docs/current/transaction-iso.html#XACT-SERIALIZABLE) are sufficient to avoid
problems with concurrent transactions.

There are, however, cases where this may not be enough and Postgres won't be tracking complex
dependencies in your operations provided they don't form
so-called [dangerous structures](https://www.sciencedirect.com/science/article/abs/pii/S0306437916300461).

When transaction T<sub>1</sub> writes to tables R<sub>w</sub> are logically dependent
on reads from R<sub>r</sub>, Postgres won't be able to make detect a conflict with another
transaction T<sub>2</sub> that may have written to R<sub>r</sub> after T<sub>1</sub> read from it
if T<sub>2</sub> would commit changes sooner than T<sub>1</sub>. This may result in a state that is
logically inconsistent if operations on entities are deemed unrelated.

For this, we introduce a limited, experimental tooling for _linearizing_ transaction.

To quote [Jepsen](https://jepsen.io/consistency/models/linearizable):

<blockquote>
Linearizability is one of the strongest single-object consistency models, and implies that every operation appears to take place atomically, in some order, consistent with the real-time ordering of those operations: e.g., if operation A completes before operation B begins, then B should logically take effect after A.
</blockquote>

To facilitate such a mode, we introduce a rule: any write in a serializable, linearized transaction that happens
after any other transaction read from the same relation, results in an immediate serialization linearization failure.

# Quick start

To linearize, you must be in a serializable transaction first. After that, all you need to do is
invoke the following function:

```postgresql
select omni_txn.linearize();
-- to check if we're in a linearized transaction
select omni_txn.linearized();
--#> t
```

It will make current transaction linearized. It will start to intercept all mutating
statements like INSERT, UPDATE, DELETE, and MERGE.

Should a linearization failure occur, it will raise a serialization error exception with
particularetails of the failure.

This is compatible with [`omni_txn.retry`](retry.md) primitive, allowing to build effective
mechanisms for handling such constraints.

