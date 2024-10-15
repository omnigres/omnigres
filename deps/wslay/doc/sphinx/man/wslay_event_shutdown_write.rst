wslay_event_shutdown_write
==========================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_shutdown_write(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_shutdown_write` prevents the event-based API context from
sending any further data to peer.

SEE ALSO
--------

:c:func:`wslay_event_get_write_enabled`
