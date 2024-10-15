wslay_event_set_error
=====================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: void wslay_event_set_error(wslay_event_context_ptr ctx, int val)

DESCRIPTION
-----------

:c:func:`wslay_event_set_error` sets error code to tell the library
there is an error.
This function is typically used in user defined callback functions.
See the description of callback function to know which error code should
be used.

RETURN VALUE
------------

:c:func:`wslay_event_set_error` does not return value.
