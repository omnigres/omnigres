wslay_event_want_read
=====================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_want_read(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_want_read` is used to know whether the library want to
read more data from peer.

This is useful to decide whether to wait for read event in
I/O event notification functions such as :manpage:`select(2)`
and :manpage:`poll(2)`.

RETURN VALUE
------------

:c:func:`wslay_event_want_read` returns 1 if the library want to read more
data from peer, or returns 0.

SEE ALSO
--------

:c:func:`wslay_event_want_write`,
:c:func:`wslay_event_shutdown_read`,
:c:func:`wslay_event_get_read_enabled`
