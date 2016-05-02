#include "admin_client.hpp"

#include <deque>
#include <iostream>
#include <vector>
#include <thread>
#include <cstdlib>

admin_client::admin_client( boost::asio::io_service & io_service)
  : message_base( io_service)
  , _connected( false)
  , _logged_in( false)
{
}

bool
admin_client::connect( std::string const& host, std::string const& port) {
  if ( !_connected) {
    _do_connect( host, port)
      .then( [ this ] ( bool connected) {
        _connected = connected;
      }).get();
  }

  if ( _connected && !_logged_in) {
    _do_login()
      .then( [ this ] ( bool logged_in) {
        _logged_in = logged_in;
      }).get();
  }

  return _connected && _logged_in;
}

bool
admin_client::run_command( std::string const& command, std::string & response, int timeout) {
  if ( !_connected) {
    return false;
  }

  return _do_run_command( command, timeout)
           .then( [ &response ] ( admin_message::buffer_type const& rsp) {
             response = rsp;
             return !response.empty();
           }).get();
}

folly::Future<bool>
admin_client::_do_connect( std::string const& host, std::string const& port) {
  return _async_connect( host, port)
           .onError( [] ( std::exception const& e) {
             std::cout << "admin_client::_do_connect() - error: " << e.what() << std::endl;
             return false;
           })
           .onTimeout( std::chrono::seconds( 5), [] () {
             std::cout << "admin_client::_do_connect() - timeout" << std::endl;
             return false;
           });
}

folly::Future<bool>
admin_client::_do_login() {
  std::string login = "matt@mba-rx";

  return _do_run_command( login, 5)
          .then( [] ( admin_message::buffer_type const& rsp) {
            std::cout << rsp << std::endl;
            if ( !rsp.empty() && rsp.find( "matt") != std::string::npos) {
              return true;
            }
            throw std::runtime_error( "Unable to login");
          })
          .onError( [] ( std::exception const& e) {
            std::cout << "admin_client::_do_login() - error: " << e.what() << std::endl;
            return false;
          })
          .onTimeout( std::chrono::seconds( 10), [] () {
            std::cout << "admin_client::_do_login() - timeout" << std::endl;
            return false;
          });
}

folly::Future<admin_message::buffer_type>
admin_client::_do_run_command( admin_message::buffer_type const& command, int timeout) {
  return _async_write( command)
           .then( &admin_client::_async_read_header, static_cast<message_base<admin_message> *>( this))
           .then( &admin_client::_async_read_body, static_cast<message_base<admin_message> *>( this))
           .onError( [] ( std::exception const& e) {
             std::cout << "admin_client::_do_run_command - error: " << e.what() << std::endl;
             return admin_message::buffer_type( "");
           })
           .onTimeout( std::chrono::seconds( 5), [] () {
             std::cout << "admin_client::_do_run_command - timeout" << std::endl;
             return admin_message::buffer_type( "");
           });
}

int main(int argc, char* argv[]) {
  std::thread t;
  boost::asio::io_service io_service;
  auto work = std::unique_ptr<boost::asio::io_service::work>( new boost::asio::io_service::work( io_service));
  t = std::thread( [ &io_service ] () { io_service.run(); });

  try {
    if ( argc != 3) {
      std::cerr << "Usage: admin_client <host> <port>\n";
      work.reset();
      t.join();
      return 1;
    }

    auto c =  std::unique_ptr<admin_client>( new admin_client( io_service));

    if ( !c->connect( argv[ 1 ], argv[ 2 ])) {
      throw std::runtime_error( "Unable to connect and throwing");
    }

    std::string response;
    if ( !c->run_command( "hello", response, 5)) {
      throw std::runtime_error( "Unable to run command and throwing");
    }
  } catch ( std::exception const& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  work.reset();
  t.join();

  return 0;
}
