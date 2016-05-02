#pragma once

#include "message_base.hpp"
#include "admin_message.hpp"

#include <tuple>

class admin_client : public message_base<admin_message>
{
public:
  admin_client( boost::asio::io_service & io_service);

  bool connect( std::string const& host, std::string const& port);
  bool run_command( std::string const& command, std::string & response, int timeout);

private:
  folly::Future<bool> _do_connect( std::string const& host, std::string const& port);
  folly::Future<bool> _do_login();

  folly::Future<admin_message::buffer_type> _do_run_command( admin_message::buffer_type const& command, int timeout);

  bool                         _connected;
  bool                         _logged_in;
};
