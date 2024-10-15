wslay_event_recv
================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_recv(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_recv` receives messages from peer.
When receiving messages, it uses :c:type:`wslay_event_recv_callback`
function. Single call of this function receives multiple messages
until :c:type:`wslay_event_recv_callback` function
sets error code ``WSLAY_ERR_WOULDBLOCK``.

:c:func:`wslay_event_recv` calls following callback functions for
their own event (See :c:func:`wslay_event_context_server_init` or
:c:func:`wslay_event_context_client_init` for the details of each
callbacks).

:c:type:`wslay_event_on_frame_recv_start_callback`
   Called when a new frame starts to be received.

:c:type:`wslay_event_on_frame_recv_start_callback`
   Called when a new frame starts to be received.

:c:type:`wslay_event_on_frame_recv_chunk_callback`
   Called when a chunk of frame payload is received.

:c:type:`wslay_event_on_frame_recv_end_callback`
   Called when a frame is completely received.

:c:type:`wslay_event_on_msg_recv_callback`
   Called when a message is completely received.

When close control frame is received, this function automatically queues
close control frame.
Also this function calls :c:func:`wslay_event_set_read_enabled`
with second argument 0 to disable further read from peer.

When ping control frame is received, this function automatically queues
pong control frame.

In case of a fatal error which leads to negative return code,
this function calls :c:func:`wslay_event_set_read_enabled` with second argument
0 to disable further read from peer.

RETURN VALUE
------------

:c:func:`wslay_event_recv` returns 0 if it succeeds, or one of the following
negative error codes:

**WSLAY_ERR_CALLBACK_FAILURE**
   User defined callback function is failed.

**WSLAY_ERR_NOMEM**
   Out of memory.

When negative error code is returned, application must not make any further
call of :c:func:`wslay_event_recv` and must close WebSocket connection.

SEE ALSO
--------

:c:func:`wslay_event_set_read_enabled`
