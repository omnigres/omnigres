wslay_event_shutdown_read
=========================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_shutdown_read(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_shutdown_read` prevents the event-based API context from
reading any further data from peer.

This function may be used with :c:func:`wslay_event_queue_close` if
the application detects error in the data received and wants to fail
WebSocket connection.

SEE ALSO
--------

:c:func:`wslay_event_get_read_enabled`
