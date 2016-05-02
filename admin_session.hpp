#pragma once

#include "message_base.hpp"
#include <memory>

class admin_session : public message_base<admin_message>, std::enable_shared_from_this<admin_session>
{
public:
  admin_session( boost::asio::ip::tcp::socket socket, std::shared_ptr<std::vector<admin_session>> sessions);
  ~admin_session();

  void start();
  void stop();

private:
  static std::string DefaultResponse() { return "unable to complete request"; }

  boost::asio::io_service &                   _io_service;
  boost::asio::ip::tcp::socket                _socket;
  std::shared_ptr<std::vector<admin_session>> _sessions;
};
