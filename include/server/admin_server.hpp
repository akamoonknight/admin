#pragma once

#include "admin_session.hpp"

#include <set>

class connection
{
public:
  connection( boost::asio::ip::tcp::socket socket)
  {
  }

private:
};

using connection_ptr = std::shared_ptr<connection>;

class connection_manager
{
public:
  connection_manager() {}

  void start( connection_ptr c) {
    _connections.insert( c);
    c->start();
  }

  void stop( connection_ptr c) {
    if ( _connections.find( c) != _connections.end()) {
      _connections.erase( c);
      c->stop();
    }
  }

  void stop_all() {
    for ( auto c : _connections) {
      c->stop();
    }
    _connections.clear();
  }

private:
  std::set<connection_ptr> _connections;
};

class admin_server
{
public:
  admin_server( boost::asio::io_service & io_service);

  bool listen( std::string const& port);
  void stop();

private:
  void _do_accept();
  
  boost::asio::io_service &      _io_service;
  boost::asio::ip::tcp::socket   _socket;
  boost::asio::ip::tcp::acceptor _acceptor;

  connection_manager _connection_manager;
};
