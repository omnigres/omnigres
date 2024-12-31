# Quickstart

??? tip "This is a template extension"

    This means `omni_rest` extension doesn't have to be installed at all times to use functions from it.

    If you'd like to be able multiple versions of `omni_rest`, or use it without extension provisioned for any other reason, you can instantiate it into a schema of your choosing:

    ```postgresql
    select omni_rest.instantiate(schema => 'your_schema_name');
    ```

# Integration

To integrate `omni_rest` with your Omnigres extension, augment your `omni_httpd.handler` to call[^call] `omni_rest.postgrest(req, outcome)` procedure. The return value for the outcome of its processing will be saved into the `outcome` parameter as it is an `inout` parameter.

The default settings are _intentionally_ strict to avoid exposing something not meant for exposure. As instructed above, no relation will be exposed.

In order to expose relations in a schema, you need to supply a third (optional) parameter of `settings`, denoting allowed schemas:

```postgresql
call omni_rest.postgrest(req, outcome, 
              omni_rest.postgrest_settings(schemas => '{app}'));
```

In the future, more settings will be added.


[^call]:

    Call as a procedure:

    ```postgresql
    call omni_rest.postgrest(req, outcome, ...)
    ```