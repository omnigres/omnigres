wslay_event_get_write_enabled
=============================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_get_write_enabled(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_get_write_enabled` returns 1 if the event-based API
context allows write operation, or return 0.

After :c:func:`wslay_event_shutdown_write` is called,
:c:func:`wslay_event_get_write_enabled` returns 0.

SEE ALSO
--------

:c:func:`wslay_event_shutdown_write`
