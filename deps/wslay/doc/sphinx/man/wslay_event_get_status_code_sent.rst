wslay_event_get_status_code_sent
================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: uint16_t wslay_event_get_status_code_sent(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_get_status_code_sent` returns status code sent
in close control frame.
If no close control frame has not been sent, returns
``WSLAY_CODE_ABNORMAL_CLOSURE``.
If sent close control frame has no status code,
returns ``WSLAY_CODE_NO_STATUS_RCVD``.

SEE ALSO
--------

:c:func:`wslay_event_get_status_code_received`
