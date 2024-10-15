wslay_event_send
================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_send(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_send` sends queued messages to peer.
When sending a message, it uses
:c:type:`wslay_event_send_callback` function.
Single call of :c:func:`wslay_event_send` sends multiple messages
until :c:type:`wslay_event_send_callback` sets error code
``WSLAY_ERR_WOULDBLOCK``.

If *ctx* is initialized for WebSocket client use,
:c:func:`wslay_event_send` uses :c:type:`wslay_event_genmask_callback`
to get new mask key.

When a message queued using :c:func:`wslay_event_queue_fragmented_msg` is sent,
:c:func:`wslay_event_send` invokes
:c:type:`wslay_event_fragmented_msg_callback` for that message.

After close control frame is sent, this function 
calls :c:func:`wslay_event_set_write_enabled` with second argument
0 to disable further transmission to peer.

If there are any pending messages, :c:func:`wslay_event_want_write`
returns 1, otherwise returns 0.

In case of a fatal error which leads to negative return code,
this function calls :c:func:`wslay_event_set_write_enabled` with second argument
0 to disable further transmission to peer.

RETURN VALUE
------------

:c:func:`wslay_event_send` returns 0 if it succeeds, or one of the following
negative error codes:

.. describe:: WSLAY_ERR_CALLBACK_FAILURE

   User defined callback function is failed.

.. describe:: WSLAY_ERR_NOMEM

   Out of memory.

When negative error code is returned, application must not make any further
call of :c:func:`wslay_event_send` and must close WebSocket connection.

SEE ALSO
--------

:c:func:`wslay_event_queue_fragmented_msg`,
:c:func:`wslay_event_set_write_enabled`,
:c:func:`wslay_event_want_write`
:c:func:`wslay_event_write`
