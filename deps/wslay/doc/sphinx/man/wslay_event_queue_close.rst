wslay_event_queue_close
=======================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_queue_close(wslay_event_context_ptr ctx, uint16_t status_code, const uint8_t *reason, size_t reason_length)

DESCRIPTION
-----------

:c:func:`wslay_event_queue_close` queues close control frame.
This function is provided just for convenience.
:c:func:`wslay_event_queue_msg` can queue a close control frame as well.
*status_code* is the status code of close control frame.
*reason* is the close reason encoded in UTF-8.
*reason_length* is the length of *reason* in bytes.
*reason_length* must be less than 123 bytes.

If *status_code* is 0, *reason* and *reason_length* is not used and
close control frame with zero-length payload will be queued.

This function just queues a message and does not send it.
:c:func:`wslay_event_send` function call sends these queued messages.

RETURN VALUE
------------

:c:func:`wslay_event_queue_close` returns 0 if it succeeds, or returns
the following negative error codes:

**WSLAY_ERR_NO_MORE_MSG**
  Could not queue given message. The one of
  possible reason is that close control frame has been
  queued/sent and no further queueing message is not allowed.

**WSLAY_ERR_INVALID_ARGUMENT**
  The given message is invalid.

**WSLAY_ERR_NOMEM**
  Out of memory.

SEE ALSO
--------

:c:func:`wslay_event_queue_msg`,
:c:func:`wslay_event_queue_fragmented_msg`
