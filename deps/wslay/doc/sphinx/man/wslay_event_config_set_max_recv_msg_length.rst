wslay_event_config_set_max_recv_msg_length
==========================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_config_set_max_recv_msg_length(wslay_event_context_ptr ctx, uint64_t val)

DESCRIPTION
-----------

:c:func:`wslay_event_config_set_max_recv_msg_length` sets maximum length of
a message that can be received.
The length of message is checked by :c:func:`wslay_event_recv` function.
If the length of a message is larger than this value,
reading operation is disabled
(same effect with :c:func:`wslay_event_shutdown_read` call)
and close control frame with ``WSLAY_CODE_MESSAGE_TOO_BIG`` is queued.
If buffering for non-control frames is disabled, the library checks
each frame payload length and does not check length of entire message.

The default value is ``(1u << 31)-1``.

SEE ALSO
--------

:c:func:`wslay_event_recv`,
:c:func:`wslay_event_shutdown_read`
