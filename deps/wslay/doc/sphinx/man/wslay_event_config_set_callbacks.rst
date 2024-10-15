wslay_event_config_set_callbacks
================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_config_set_callbacks(wslay_event_context_ptr ctx, const struct wslay_event_callbacks *callbacks)

DESCRIPTION
-----------

:c:func:`wslay_event_config_set_callbacks` sets callbacks to *ctx*.
The callbacks previously set by this function or
:c:func:`wslay_event_context_server_init` or
:c:func:`wslay_event_context_client_init` are replaced with *callbacks*.

SEE ALSO
--------

:c:func:`wslay_event_context_server_init`,
:c:func:`wslay_event_context_client_init`
