wslay_event_config_set_no_buffering
===================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_config_set_no_buffering(wslay_event_context_ptr ctx, int val)

DESCRIPTION
-----------

:c:func:`wslay_event_config_set_no_buffering`
enables or disables buffering of an entire message for non-control frames.
If *val* is 0, buffering is enabled.
Otherwise, buffering is disabled.
If :c:type:`wslay_event_on_msg_recv_callback` is invoked when buffering is
disabled, the *msg_length* member of *struct wslay_event_on_msg_recv_arg*
is set to 0.

The control frames are always buffered regardless of this function call.

This function must not be used after the first invocation of
:c:func:`wslay_event_recv` function.

SEE ALSO
--------

:c:func:`wslay_event_recv`
