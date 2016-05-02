#include "client/admin_client.hpp"

#include <deque>
#include <iostream>
#include <vector>
#include <thread>
#include <cstdlib>

// All the asynchronous wrappers for the boost commands are inside of message_base-inl.hpp
// It takes a message type that tells it how to format and parse bytes as a template argument.
// I think this is quite extensible and could be used for most things that have a header and
// a body, which I would imagine is most things we will be sending, could definitely use some
// refinement in regards to more general underlying message types though probably.
//
// Mostly this class uses futures and continuations. All of the 'thens' attach a new future that
// relies on the result of the previous future and returns a new future with a new result. In
// this way asynchronous operations can be chained together in a logical way.
//
// Most of this code ends up being sychronous, but it should be relatively easy to create asynchronous
// behaviour from the existing code if the user of this code decides it can handle asynchronicity.
// This type of code should alse be far easier to extend and add features to than corresponding
// callback based code. At least in my opinion.

admin_client::admin_client( boost::asio::io_service & io_service)
  : message_base( io_service)
  , _connected( false)
  , _logged_in( false)
{
}

bool
admin_client::connect( std::string const& host, std::string const& port) {
  if ( !_connected) {
    // Do connect and get.
    // If we wanted to split up the connect and login 
    // phases and give the user more control over the
    // timing of those operations, it'd be pretty easily
    // doable.
    _do_connect( host, port)
      .then( [ this ] ( bool connected) {
        _connected = connected;
      }).get();
  }

  if ( _connected && !_logged_in) {
    // do login catches exceptions, and returns
    // whether the logging in process completed
    _do_login()
      .then( [ this ] ( bool logged_in) {
        _logged_in = logged_in;
      }).get();
  }

  return _connected && _logged_in;
}

bool
admin_client::run_command( std::string const& command, std::string & response, int timeout) {
  if ( !_connected || !_logged_in) {
    return false;
  }

  // do run command catches exceptions, just
  // assign response and return whether anything
  // got filled out
  return _do_run_command( command, timeout)
           .then( [ &response ] ( admin_message::buffer_type const& rsp) {
             response = rsp;
             return !response.empty();
           }).get();
}

folly::Future<bool>
admin_client::_do_connect( std::string const& host, std::string const& port) {
  // async connect will try to connect, could
  // potentially throw an exception or timeout
  return _async_connect( host, port)
           // standard error and timeout handling
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

  // login is backed by a run command.
  // need to see 'Login' in response
  return _do_run_command( login, 5)
          // check if the response contains required information
          .then( [] ( admin_message::buffer_type const& rsp) {
            if ( !rsp.empty() && rsp.find( "Login") != std::string::npos) {
              return true;
            }
            throw std::runtime_error( "Unable to login");
          })
          // standard error and timeout handling
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
  // first do a write of the command
  return _async_write( command)
           // then read the header (there needs to be a static cast because of folly not liking somethign about the bind to _async_read_header)
           .then( &admin_client::_async_read_header, static_cast<message_base<admin_message> *>( this))
           // then read the body (there needs to be a static cast because of folly not liking somethign about the bind to _async_read_header)
           .then( &admin_client::_async_read_body, static_cast<message_base<admin_message> *>( this))
           // standard error and timeout handling
           .onError( [] ( std::exception const& e) {
             std::cout << "admin_client::_do_run_command - error: " << e.what() << std::endl;
             return admin_message::buffer_type( "");
           })
           .onTimeout( std::chrono::seconds( 5), [] () {
             std::cout << "admin_client::_do_run_command - timeout" << std::endl;
             return admin_message::buffer_type( "");
           });

  // The lovely part about this is that it should be relatively easy to
  // add things like persistence or logging, they would just be extra
  // then commands in the chain. Easy to read, easy to reason about,
  // I'm not sure on the latency aspects, but the maintainability is
  // top-notch I'd think.
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
