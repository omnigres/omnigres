wslay_event_get_close_received
==============================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_get_close_received(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_get_close_received` returns 1 if a close control frame
has been received from peer, or returns 0.

SEE ALSO
--------

:c:func:`wslay_event_get_close_sent`
