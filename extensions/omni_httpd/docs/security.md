# Security


!!! warning "HTTP server hardening"

    At this moment, this extension does not provide any additional
    hardening for the HTTP server functionality to prevent any unintended
    interaction between the server and the database outside of strict confines
    of the message passing approach used for their intended way of communication.

    We are eager to add support for such hardening (perhaps as an opt-in if it 
    significantly decreases performance). Please consider
    [contributing](https://github.com/omnigres/omnigres/pulls).
    
`omni_httpd` relies on Postgres security primitives. In order to enforce a role on
a handler, it must be made `security definer` and has to be owned by the role it is
intended to be running under.