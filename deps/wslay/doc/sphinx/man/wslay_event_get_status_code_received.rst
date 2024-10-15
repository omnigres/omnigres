wslay_event_get_status_code_received
====================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: uint16_t wslay_event_get_status_code_received(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_get_status_code_received` returns status code
received in close control frame.
If no close control frame has not been received, returns
``WSLAY_CODE_ABNORMAL_CLOSURE``.
If received close control frame has no status code,
returns ``WSLAY_CODE_NO_STATUS_RCVD``.

SEE ALSO
--------

:c:func:`wslay_event_get_status_code_sent`
