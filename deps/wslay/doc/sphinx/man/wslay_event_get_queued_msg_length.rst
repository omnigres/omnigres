wslay_event_get_queued_msg_length
=================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: size_t wslay_event_get_queued_msg_length(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_get_queued_msg_length` returns the sum of queued message
length.
It only counts the message length queued using
:c:func:`wslay_event_queue_msg` or :c:func:`wslay_event_queue_close`.
