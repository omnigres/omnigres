.. highlight:: c

wslay_event_context_server_init, wslay_event_context_client_init, wslay_event_context_free
==========================================================================================

SYNOPSIS
--------

#include <wslay/wslay.h>

.. c:function:: int wslay_event_context_server_init(wslay_event_context_ptr *ctx, const struct wslay_event_callbacks *callbacks, void *user_data)
.. c:function:: int wslay_event_context_client_init(wslay_event_context_ptr *ctx, const struct wslay_event_callbacks *callbacks, void *user_data)
.. c:function:: void wslay_event_context_free(wslay_event_context_ptr ctx)

DESCRIPTION
-----------

:c:func:`wslay_event_context_server_init` function initializes an
event-based API context for WebSocket server use.
:c:func:`wslay_event_context_client_init` function initializes an
event-based API context for WebSocket client use.  If they return
successfully, `ctx` will point to a structure which holds any
necessary resources needed to process WebSocket protocol transfers.

*callbacks* is a pointer to :c:type:`wslay_event_callbacks`,
which is defined as follows::

  struct wslay_event_callbacks {
      wslay_event_recv_callback                recv_callback;
      wslay_event_send_callback                send_callback;
      wslay_event_genmask_callback             genmask_callback;
      wslay_event_on_frame_recv_start_callback on_frame_recv_start_callback;
      wslay_event_on_frame_recv_chunk_callback on_frame_recv_chunk_callback;
      wslay_event_on_frame_recv_end_callback   on_frame_recv_end_callback;
      wslay_event_on_msg_recv_callback         on_msg_recv_callback;
  };

.. c:type:: ssize_t (*wslay_event_recv_callback)(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags, void *user_data)

   *recv_callback* is invoked by :c:func:`wslay_event_recv` when it
   wants to receive more data from peer.
   The implementation of this callback function must read data at most *len*
   bytes from peer and store them in *buf* and return the number of bytes read.
   *flags* is always 0 in this version.

   If there is an error, return -1 and
   set error code ``WSLAY_ERR_CALLBACK_FAILURE``
   using :c:func:`wslay_event_set_error`.
   Wslay event-based API on the whole assumes non-blocking I/O.
   If the cause of error is ``EAGAIN`` or ``EWOULDBLOCK``,
   set ``WSLAY_ERR_WOULDBLOCK`` instead. This is important because it tells
   :c:func:`wslay_event_recv` to stop receiving further data and return.

.. c:type:: ssize_t (*wslay_event_send_callback)(wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)

   *send_callback* is invoked by :c:func:`wslay_event_send` when it
   wants to send more data to peer.
   The implementation of this callback function must send data at most *len*
   bytes to peer and return the number of bytes sent.
   *flags* is the bitwise OR of zero or more of the following flag:

   ``WSLAY_MSG_MORE``
     There is more data to send

   It provides some hints to tune performance and behaviour.

   If there is an error, return -1 and
   set error code ``WSLAY_ERR_CALLBACK_FAILURE``
   using :c:func:`wslay_event_set_error`.
   Wslay event-based API on the whole assumes non-blocking I/O.
   If the cause of error is ``EAGAIN`` or ``EWOULDBLOCK``,
   set ``WSLAY_ERR_WOULDBLOCK`` instead. This is important because it tells
   :c:func:`wslay_event_send` to stop sending data and return.

.. c:type:: int (*wslay_event_genmask_callback)(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)

   *genmask_callback* is invoked by :c:func:`wslay_event_send` when it
   wants new mask key. As described in RFC6455, only the traffic from
   WebSocket client is masked, so this callback function is only needed
   if an event-based API is initialized for WebSocket client use.
   The implementation of this callback function must fill exactly *len* bytes
   of data in *buf* and return 0 on success.
   If there is an error, return -1 and
   set error code ``WSLAY_ERR_CALLBACK_FAILURE``
   using :c:func:`wslay_event_set_error`.

.. c:type:: void (*wslay_event_on_frame_recv_start_callback)(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg, void *user_data)

   *on_frame_recv_start_callback* is invoked by :c:func:`wslay_event_recv` when
   a new frame starts to be received.
   This callback function is only invoked once for each
   frame. :c:type:`wslay_event_on_frame_recv_start_arg` is defined as
   follows::

     struct wslay_event_on_frame_recv_start_arg {
         uint8_t  fin;
         uint8_t  rsv;
         uint8_t  opcode;
         uint64_t payload_length;
     };

   *fin*, *rsv* and *opcode* is fin bit and reserved bits and opcode of a frame.
   *payload_length* is a payload length of a frame.

.. c:type:: void (*wslay_event_on_frame_recv_chunk_callback)(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg, void *user_data)

   *on_frame_recv_chunk_callback* is invoked by :c:func:`wslay_event_recv` when
   a chunk of frame payload is received.
   :c:type:`wslay_event_on_frame_recv_chunk_arg` is defined as follows::

     struct wslay_event_on_frame_recv_chunk_arg {
         const uint8_t *data;
         size_t         data_length;
     };

   *data* points to a chunk of payload data.
   *data_length* is the length of a chunk.

.. c:type:: void (*wslay_event_on_frame_recv_end_callback)(wslay_event_context_ptr ctx, void *user_data)

   *on_frame_recv_end_callback* is invoked by :c:func:`wslay_event_recv` when
   a frame is completely received.

.. c:type:: void (*wslay_event_on_msg_recv_callback)(wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg, void *user_data)

   *on_msg_recv_callback* is invoked by :c:func:`wslay_event_recv`
   when a message
   is completely received. :c:type:`wslay_event_on_msg_recv_arg` is
   defined as follows::

     struct wslay_event_on_msg_recv_arg {
         uint8_t        rsv;
         uint8_t        opcode;
         const uint8_t *msg;
         size_t         msg_length;
         uint16_t       status_code;
     };

   The *rsv* member and the *opcode* member are reserved bits and opcode of
   received message respectively.
   The *rsv* member is constructed as follows::

      rsv = (RSV1 << 2) | (RSV2 << 1) | RSV3

   The *msg* member points to the message of the received message.
   The *msg_length* member is the length of message.
   If a message is close control frame, in other words,
   ``opcode == WSLAY_CONNECTION_CLOSE``, *status_code* is set to
   the status code in the close control frame.
   If no status code is included in the close control frame,
   *status_code* set to 0.

*user_data* is an arbitrary pointer, which is directly
passed to each callback functions as *user_data* argument.

When initialized event-based API context *ctx* is no longer used,
use :c:func:`wslay_event_context_free` to free any
resources allocated for *ctx*.

RETURN VALUE
------------

:c:func:`wslay_event_context_server_init` and
:c:func:`wslay_event_context_client_init` returns 0 if it succeeds,
or one of the following negative error codes:

.. describe:: WSLAY_ERR_NOMEM

   Out of memory.

SEE ALSO
--------

:c:func:`wslay_event_send`, :c:func:`wslay_event_recv`,
:c:func:`wslay_event_set_error`
