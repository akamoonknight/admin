#include "admin_session.hpp"

admin_session::admin_session( boost::asio::ip::tcp::socket socket)
  : _io_service( io_service)
  , _socket( std::move( socket))
{
}

admin_session::~admin_session() {
}

void
admin_session::start() {
  _io_service.post( [ this ] () {
    // expect login from client, time out after 60 seconds
    _logged_in = _async_read_header()
      .then( &admin_client::_async_read_body, static_cast<message_base<admin_message *>( this))
      .then( [] ( buffer_type const& request) {
        std::cout << "Got login from '" << request << "'" << std::endl;
      })
      .then( &admin_session::_async_write, static_cast<message_base<admin_message> *>( this), _admin->help())
      .then( [] () {
        return true;
      });
      .onError( [] ( std::exception const& e) {
        std::cout << "admin_session::start() - login error" << std::endl;
        return false;
      })
      .onTimeout( std::chrono::seconds( 60), [] () {
        std::cout << "admin_session::start() - login timed out" << std::endl;
        return false;
      }).get();

    while ( _logged_in) {
      // Get request from connection, time out if no request is made within 30 minutes
      auto request = _async_read_header()
        .then( &admin_client::_async_read_body, static_cast<message_base<admin_message *>( this))
        .onError( [] ( std::exception const& e) {
          std::cout << "admin_session::start() - request error" << std::endl;
          return std::string( "");
        })
        .onTimeout( std::chrono::minutes( 30), [] () {
          std::cout << "admin_session::start() - request timed out" << std::endl;
          return std::string( "");
        }).get();

      // Check for request status
      if ( request.empty() || request == "exit") {
        std::cout << "admin_session::start() - session exited" << std::endl;
        break;
      }

      // Send request through the admin interface, time out if no response is
      // made within 5 minutes
      auto response = _do_admin_command( request)
        .onError( [] ( std::exception const& e) {
          std::cout << "admin_session::start() - admin command error: " << e.what() << std::endl;
          return DefaultResponse();
        })
        .onTimeout( std::chrono::minutes( 5), [] () {
          std::cout << "admin_session::start() - admin command timed out, sending default" << std::endl;
          return DefaultResponse();
        });

      // Write the response to the underlying transport
      _async_write( response)
        .onError( [] ( std::exception const& e) {
          std::cout << "admin_session::start() - response error: " << e.what() << std::endl;
        })
        .onTimeout( std::chrono::seconds( 5), [] () {
          std::cout << "admin_session::start() - response timed out" << std::endl;
        }).get();
      }
    }
  });
}

void
admin_session::stop() {
  _io_service.post( [ this ] () {
    _logged_in = false;
  });
}

folly::Future<std::string>
admin_session::_do_admin_command( std::string const& request) {
  auto p = std::make_shared<folly::Promise<std::string>>();

  p->setValue( "response");

  return p->getFuture();
}
