# Session Management

`omni_session` extension brings standardized session management to the Omnigres stack. Currently it is focused on
providing this functionality in cooperation with the HTTP stack, but theoretically it is not limited to it.

This extensions creates an unlogged `omni_session.sessions` table that contains all sessions.
