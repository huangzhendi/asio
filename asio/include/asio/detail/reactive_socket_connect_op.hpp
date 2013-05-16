//
// detail/reactive_socket_connect_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_REACTIVE_SOCKET_CONNECT_OP_HPP
#define ASIO_DETAIL_REACTIVE_SOCKET_CONNECT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"
#include "asio/detail/addressof.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/buffer_sequence_adapter.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/reactor_op.hpp"
#include "asio/detail/socket_ops.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Protocol>
class reactive_socket_connect_op_base : public reactor_op
{
public:
  reactive_socket_connect_op_base(socket_type socket,
      const typename Protocol::endpoint& peer_endpoint, func_type complete_func)
    : reactor_op(&reactive_socket_connect_op_base::do_perform, complete_func),
      socket_(socket),
      peer_endpoint_(peer_endpoint)
  {
  }

  static bool do_perform(reactor_op* base)
  {
    reactive_socket_connect_op_base* o(
        static_cast<reactive_socket_connect_op_base*>(base));

    return socket_ops::non_blocking_connect(o->socket_,
        o->peer_endpoint_.data(), o->peer_endpoint_.size(), o->ec_);
  }

private:
  socket_type socket_;
  typename Protocol::endpoint peer_endpoint_;
};

template <typename Protocol, typename Handler>
class reactive_socket_connect_op :
  public reactive_socket_connect_op_base<Protocol>
{
public:
  ASIO_DEFINE_HANDLER_PTR(reactive_socket_connect_op);

  reactive_socket_connect_op(socket_type socket,
      const typename Protocol::endpoint& peer_endpoint, Handler& handler)
    : reactive_socket_connect_op_base<Protocol>(socket, peer_endpoint,
        &reactive_socket_connect_op::do_complete),
      handler_(ASIO_MOVE_CAST(Handler)(handler))
  {
  }

  static void do_complete(io_service_impl* owner, operation* base,
      const asio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    reactive_socket_connect_op* o
      (static_cast<reactive_socket_connect_op*>(base));
    ptr p = { asio::detail::addressof(o->handler_), o, o };

    ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder1<Handler, asio::error_code>
      handler(o->handler_, o->ec_);
    p.h = asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      asio_handler_invoke_helpers::invoke(handler, handler);
      ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  Handler handler_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_REACTIVE_SOCKET_CONNECT_OP_HPP
