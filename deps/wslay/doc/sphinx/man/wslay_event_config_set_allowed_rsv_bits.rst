.. highlight:: c

wslay_event_config_set_allowed_rsv_bits
=======================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_config_set_allowed_rsv_bits(wslay_event_context_ptr ctx, uint8_t rsv)

DESCRIPTION
-----------

:c:func:`wslay_event_config_set_allowed_rsv_bits` sets a bit mask of
allowed reserved bits (RSVs).  Currently only permitted values are
``WSLAY_RSV1_BIT`` to allow PMCE extension (see :rfc:`7692`) or
``WSLAY_RSV_NONE`` to disable.

When bits other than ``WSLAY_RSV1_BIT`` is set in *rsv*, those bits
are simply ignored.

The set value is used to check the validity of reserved bits for both
transmission and reception of WebSocket frames.

RETURN VALUE
------------

This function always succeeds, and there is no return value.

SEE ALSO
--------

:c:func:`wslay_event_queue_fragmented_msg`,
:c:func:`wslay_event_queue_fragmented_msg_ex`
