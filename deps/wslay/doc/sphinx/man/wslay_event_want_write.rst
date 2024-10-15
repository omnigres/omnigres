wslay_event_want_write
======================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_want_write(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_want_write` is used to know whether the library want to
send more data to peer.

This is useful to decide whether to wait for write event in
I/O event notification functions such as :manpage:`select(2)`
and :manpage:`poll(2)`.

RETURN VALUE
------------

:c:func:`wslay_event_want_write` returns 1 if the library want to send more
data to peer, or returns 0.

SEE ALSO
--------

:c:func:`wslay_event_want_read`,
:c:func:`wslay_event_shutdown_write`,
:c:func:`wslay_event_get_write_enabled`
