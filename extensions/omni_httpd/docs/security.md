<!-- @formatter:off -->

# Security


!!! warning "HTTP server hardening"

    At this moment, this extension does not provide any additional
    hardening for the HTTP server functionality to prevent any unintended
    interaction between the server and the database outside of strict confines
    of the message passing approach used for their intended way of communication.

    We are eager to add support for such hardening (perhaps as an opt-in if it 
    significantly decreases performance). Please consider
    [contributing](https://github.com/omnigres/omnigres/pulls).
    


## Handler Queries

The security model behind handler query execution relies on the
`role` column in the `handlers` table. It can be set only
to the role that is "accessible" to the current user (meaning either
it is the same role or the current user can set this role given its
permissions.)

Each request will be executed with this role as a _security
restricted mode that disallows `SET ROLE`_ (`SECURITY_LOCAL_USERID_CHANGE`)[^unless-superuser],
prevent the code to elevate its privileges.

[^unless-superuser]: unless this role is a superuser itself