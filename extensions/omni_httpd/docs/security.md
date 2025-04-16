# Security


!!! warning "HTTP server hardening"

    At this moment, this extension does not provide any additional
    hardening for the HTTP server functionality to prevent any unintended
    interaction between the server and the database outside of strict confines
    of the message passing approach used for their intended way of communication.

    We are eager to add support for such hardening (perhaps as an opt-in if it 
    significantly decreases performance). Please consider
    [contributing](https://github.com/omnigres/omnigres/pulls).

In order to enforce a role on a handler, use `omni_httpd.handler` table to specify a mapping of a handler
to a specific role.

!!! warning "Sandboxing procedural handlers with roles"

    **Please note** that currently there's an outstanding issue of procedures (as opposed to functions)
    still allowing for accidental or deliberate role changes within. One of the main reasons to use
    procedures is their capability to be non-atomic. However, due to current Postgres' design,
    this prevents us from using security contexts in procedures.

    So, code like this will change the role to `another_role`, regardless of pre-configured role,
    breaking out of the sandbox of the specified role.

    ```postgresql
     create or replace procedure my_handler(outcome out omni_httpd.http_outcome)
        language plpgsql as
    $$
    begin
        --- ....
        set role another_role;
        --- ...
    end;
    $$ 
    ```

    Functions are **not** subject to this limitation.
