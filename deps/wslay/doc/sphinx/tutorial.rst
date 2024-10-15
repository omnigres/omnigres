.. highlight:: c

Tutorial
========

WebSocket Echo Server
---------------------

This section describes briefly how to make WebSocket echo server with
Wslay library in C.
The complete source code is located at :file:`examples/fork-echoserv.c`.

This WebSocket echo server listens on port given in the command-line.
When the incoming connection from the client is accepted,
it forks another process.
The parent process goes back to the event loop and can accept another client.
The child process communicates with
the client (WebSocket HTTP handshake and then WebSocket data transfer).

For the purpose of this tutorial, we focus on the use of Wslay library.
The primary function to communicate with WebSocket client is
:c:func:`communicate` function.

.. c:function:: int communicate(int fd)

This function performs HTTP handshake
and WebSocket data transfer until close handshake is done or an
error occurs. *fd* is the file descriptor of the connection to the
client. This function returns 0 if it succeeds, or returns 0.

Let's look into this function. First we perform HTTP handshake.
It will be done with :c:func:`http_handshake` function.
When it succeeds, we make the file descriptor of the connection non-block.
You may set other socket options like ``TCP_NODELAY``.
At this point, we can start WebSocket data transfer.

Now establish callbacks for wslay event-based API.
We use 3 callbacks in this example::

  struct wslay_event_callbacks callbacks = {
    recv_callback,
    send_callback,
    NULL,
    NULL,
    NULL,
    NULL,
    on_msg_recv_callback
  };

``recv_callback`` is invoked by :c:func:`wslay_event_recv`
when it wants to read more data from the client.
It looks like this::

  ssize_t recv_callback(wslay_event_context_ptr ctx, uint8_t *buf, size_t len,
                        void *user_data)
  {
    struct Session *session = (struct Session*)user_data;
    ssize_t r;
    while((r = recv(session->fd, buf, len, 0)) == -1 && errno == EINTR);
    if(r == -1) {
      if(errno == EAGAIN || errno == EWOULDBLOCK) {
        wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
      } else {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
      }
    } else if(r == 0) {
      /* Unexpected EOF is also treated as an error */
      wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
      r = -1;
    }
    return r;
  }    

If :c:func:`recv` failed with ``EAGAIN`` or ``EWOULDBLOCK``
(notice that we made socket
non-block), we set ``WSLAY_ERR_WOULDBLOCK`` using
:c:func:`wslay_event_set_error`
to tell the wslay library to stop reading from the socket.
If it failed with other reasons, we set ``WSLAY_ERR_CALLBACK_FAILED``,
and it will make :c:func:`wslay_event_recv` fail.
Notice that reading EOF here is unexpected: so it is also treated as an error.

``send_callback`` is invoked by :c:func:`wslay_event_send`
when it wants to send data to the client.
It looks like this::

  ssize_t send_callback(wslay_event_context_ptr ctx,
                        const uint8_t *data, size_t len, void *user_data)
  {
    struct Session *session = (struct Session*)user_data;
    ssize_t r;

    int sflags = 0;
  #ifdef MSG_MORE
    if(flags & WSLAY_MSG_MORE) {
      sflags |= MSG_MORE;
    }
  #endif // MSG_MORE
    while((r = send(session->fd, data, len, sflags)) == -1 && errno == EINTR);
    if(r == -1) {
      if(errno == EAGAIN || errno == EWOULDBLOCK) {
        wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
      } else {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
      }
    }
    return r;
  }

Similar to ``recv_callback``, we set error code using
:c:func:`wslay_event_set_error` depending on the ``errno`` value.

``on_msg_recv_callback`` is invoked by :c:func:`wslay_event_recv`
when it have received a message completely.
It looks like this::

  void on_msg_recv_callback(wslay_event_context_ptr ctx,
                            const struct wslay_event_on_msg_recv_arg *arg,
                            void *user_data)
  {
    /* Echo back non-control message */
    if(!wslay_is_ctrl_frame(arg->opcode)) {
      struct wslay_event_msg msgarg = {
        arg->opcode, arg->msg, arg->msg_length
      };
      wslay_event_queue_msg(ctx, &msgarg);
    }
  }

Here, since we are building echo server, we just echo back non-control
frames to the client. ``arg->opcode`` is a opcode of the received
message.  ``arg->msg`` contains received message data with length
``arg->msg_length``.
:c:func:`wslay_event_queue_msg` queues message to the client.

Then initialize wslay event-based API context::

  wslay_event_context_server_init(&ctx, &callbacks, &session);

At this point, we finished initialization of Wslay library and all we have to
do is run event-loop and communicate with the client.
For event-loop we need event notification mechanism, here we use
standard :c:func:`poll`. Since we don't have any message to send client,
first we query read event only.

The event loop looks like this::

  /*
   * Event loop: basically loop until both wslay_event_want_read(ctx)
   * and wslay_event_want_write(ctx) return 0.
   */
  while(wslay_event_want_read(ctx) || wslay_event_want_write(ctx)) {
    int r;
    while((r = poll(&event, 1, -1)) == -1 && errno == EINTR);
    if(r == -1) {
      perror("poll");
      res = -1;
      break;
    }
    if(((event.revents & POLLIN) && wslay_event_recv(ctx) != 0) ||
       ((event.revents & POLLOUT) && wslay_event_send(ctx) != 0) ||
       (event.revents & (POLLERR | POLLHUP | POLLNVAL))) {
      /*
       * If either wslay_event_recv() or wslay_event_send() return
       * non-zero value, it means serious error which prevents wslay
       * library from processing further data, so WebSocket connection
       * must be closed.
       */
      res = -1;
      break;
    }
    event.events = 0;
    if(wslay_event_want_read(ctx)) {
      event.events |= POLLIN;
    }
    if(wslay_event_want_write(ctx)) {
      event.events |= POLLOUT;
    }
  }
  return res;

Basically, we just loop until both :c:func:`wslay_event_want_read` and
:c:func:`wslay_event_want_write` return 0.
Also if either :c:func:`wslay_event_recv` or :c:func:`wslay_event_send`
return non-zero value, we exit the loop.

If there is data to read, call :c:func:`wslay_event_recv`.
If there is data to write and writing will not block, call
:c:func:`wslay_event_send`.

After exiting the event loop, we just close the connection,
most likely, using ``shutdown(fd, SHUT_WR)`` and ``close(fd)``.
