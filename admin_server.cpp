#include "admin_server.hpp"

admin_server::admin_server( boost::asio::io_service & io_service)
  : _io_service( io_service)
  , _socket( io_service)
  , _acceptor( io_service)
{
}

void
admin_server::listen( std::string const& port) {
  if ( !_acceptor.is_open()) {
    auto endpoint = boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4(), port);

    _acceptor.open( endpoint.protocol());
    _acceptor.bind( endpoint);
    _acceptor.listen();

    _do_accept();
  }
}

admin_server::stop() {
  _io_service.post( [ this ] () {
    _acceptor.close();
    _connection_manager.stop_all();
  });
}

void
admin_server::_do_accept() {
  boost::asio::async_accept( _socket,
    _acceptor,
    [ this ] ( boost::system::error_code const& ec) {
      if ( !_acceptor.is_open()) { return; }

      if ( !ec) {
        _connection_manager.start( std::make_shared<connection>( std::move( _socket), _connection_manager, (admin_session_handler())));
      }

      _do_accept();
    }
  );
}
