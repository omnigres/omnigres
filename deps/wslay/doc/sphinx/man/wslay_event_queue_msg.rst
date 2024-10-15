.. highlight:: c

wslay_event_queue_msg, wslay_event_queue_msg_ex
===============================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_queue_msg(wslay_event_context_ptr ctx, const struct wslay_event_msg *arg)
.. c:function:: int wslay_event_queue_msg_ex(wslay_event_context_ptr ctx, const struct wslay_event_msg *arg, uint8_t rsv)

DESCRIPTION
-----------

:c:func:`wslay_event_queue_msg` and :c:func:`wslay_event_queue_msg_ex`
queue message specified in *arg*.  The *struct wslay_event_msg* is
defined as::

  struct wslay_event_msg {
      uint8_t        opcode;
      const uint8_t *msg;
      size_t         msg_length;
  };

The *opcode* member is opcode of the message.
The *msg* member is the pointer to the message data.
The *msg_length* member is the length of message data.

This function supports both control and non-control messages and
the given message is sent without fragmentation.
If fragmentation is needed, use :c:func:`wslay_event_queue_fragmented_msg`
function instead.

This function makes a copy of *msg* of length *msg_length*.

This function just queues a message and does not send it.
:c:func:`wslay_event_send` function call sends these queued messages.

:c:func:`wslay_event_queue_msg_ex` additionally accepts *rsv*
parameter, which is a reserved bits to send. To set reserved bits, use
macro ``WSLAY_RSV1_BIT``, ``WSLAY_RSV2_BIT``, and ``WSLAY_RSV3_BIT``.
See :c:func:`wslay_event_config_set_allowed_rsv_bits` to see the
allowed reserved bits to set.

RETURN VALUE
------------

:c:func:`wslay_event_queue_msg` and :c:func:`wslay_event_queue_msg_ex`
return 0 if it succeeds, or returns the following negative error
codes:

**WSLAY_ERR_NO_MORE_MSG**
  Could not queue given message. The one of
  possible reason is that close control frame has been
  queued/sent and no further queueing message is not allowed.

**WSLAY_ERR_INVALID_ARGUMENT**
  The given message is invalid; or RSV1 is set for control frame; or
  bit is set in *rsv* which is not allowed (see
  :c:func:`wslay_event_config_set_allowed_rsv_bits`).

**WSLAY_ERR_NOMEM**
  Out of memory.

SEE ALSO
--------

:c:func:`wslay_event_queue_fragmented_msg`,
:c:func:`wslay_event_queue_fragmented_msg_ex`,
:c:func:`wslay_event_queue_close`
