# Session Management

`omni_session` extension brings standardized session management to the Omnigres stack. Currently it is focused on
providing this functionality in cooperation with the HTTP stack, but theoretically it is not limited to it.

This extensions creates an unlogged `omni_session.sessions` table that contains all sessions.

## Session handler

Function `omni_session.session_handler` is designed to handle session application to differently typed objects:

### UUID

`omni_session.session_handler(uuid)` returns `uuid` and represents the core of session functionality.

* Given a null `uuid`, it creates a new session and sets `omni_session.session` transaction variable to it. Returns a
  new session UUID.
* Given a non-null `uuid` that is a valid existing session, it sets `omni_session.session` transaction variable to it.
  Returns the same UUID.
* Given a non-null `uuid` that is not a valid existing session, it creates a new session and sets `omni_session.session`
  transaction variable to it. Returns a new session UUID.

### HTTP Request

`omni_session.session_handler(omni_httpd.http_request)` returns unmodified `omni_httpd.http_request` and retrieves the
UUID from request's cookie called `_session`. Its behavior mirrors that of UUID behavior above.

Accepts an optional `cookie_name` parameter to specify a different name for the cookie.

### HTTP Response

`omni_session.session_handler(omni_httpd.http_response)` returns a modified `omni_httpd.http_outcome` with a cookie
`_session` set to `omni_session.session` transaction variable value.

Accepts an optional `cookie_name` parameter to specify a different name for the cookie.
