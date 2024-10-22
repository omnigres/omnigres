create function ensure_affected_accounts_balance() returns trigger
    language plpgsql
as
$$
declare
    balance omni_ledger.amount;
begin
    select sum(ab.balance)
    into balance
    from omni_ledger.account_balances ab
             inner join omni_ledger.statement_affected_accounts() tat
                        on tat.account_id = ab.account_id;
    if balance != 0 then
        raise exception 'violation: accounts did not balance: %', balance;
    end if;
    return null;
end;
$$;