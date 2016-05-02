#pragma once

#include <folly/futures/Future.h>

#include <boost/asio.hpp>

#include <string>
#include <vector>
#include <cstdint>

template <typename MessageType>
class message_base
{
public:
  using buffer_type = typename MessageType::buffer_type;

  message_base( boost::asio::io_service & io_service);
  message_base( boost::asio::ip::tcp::socket socket);
  virtual ~message_base();

protected:
  folly::Future<bool> _async_connect( std::string const& host, std::string const& port);
  //folly::Future<bool> _async_accept();

  folly::Future<folly::Unit> _async_write( buffer_type const& buffer);

  folly::Future<std::vector<char>> _async_read_header();

  folly::Future<typename MessageType::buffer_type> _async_read_body( std::vector<char> & vec);

  boost::asio::io_service &    _io_service;
  boost::asio::ip::tcp::socket _socket;
};

#include "message_base-inl.hpp"
