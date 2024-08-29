# Session Management

`omni_session` extension brings standardized session management to the Omnigres stack. Currently, it is focused on
providing this functionality in cooperation with the HTTP stack, but it is not limited to it and can be used outside
of the HTTP context.

This extensions creates an unlogged `omni_session.sessions` table that contains all sessions.

## Session handler

Function `omni_session.session_handler` is designed to handle session application to differently typed objects:

### Session ID

`omni_session.session_handler(omni_session.session_id)` returns `omni_session.session_id` and represents the core of
session functionality.

* Given a null session ID, it creates a new session and sets `omni_session.session` transaction variable to it. Returns
  a
  new session ID.
* Given a non-null session ID that is a valid existing session, it sets `omni_session.session` transaction variable to
  it.
  Returns the same session ID.
* Given a non-null session ID that is not a valid existing session, it creates a new session and sets
  `omni_session.session`
  transaction variable to it. Returns a new session ID.

### HTTP Request

`omni_session.session_handler(omni_httpd.http_request)` returns unmodified `omni_httpd.http_request` and retrieves the
UUID from request's cookie called `_session`. Its behavior mirrors that of ID behavior above.

Accepts an optional `cookie_name` parameter to specify a different name for the cookie.

### HTTP Response

`omni_session.session_handler(omni_httpd.http_response)` returns a modified `omni_httpd.http_outcome` with a cookie
`_session` set to `omni_session.session` transaction variable value.

Accepts an optional `cookie_name` parameter to specify a different name for the cookie.
