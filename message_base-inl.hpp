template <typename MessageType>
message_base<MessageType>::message_base( boost::asio::io_service & io_service)
  : _io_service( io_service)
  , _socket( io_service)
{
}

template <typename MessageType>
message_base<MessageType>::message_base( boost::asio::ip::tcp::socket socket)
  : _io_service( socket.get_io_service())
  , _socket( std::move( socket))
{
}

template <typename MessageType>
message_base<MessageType>::~message_base() {
}

// Future wrappers around boost asio async calls
template <typename MessageType>
folly::Future<bool>
message_base<MessageType>::_async_connect( std::string const& host, std::string const& port) {
  auto p = std::make_shared<folly::Promise<bool>>();

  boost::asio::async_connect( _socket,
    boost::asio::ip::tcp::resolver( _io_service).resolve( { host, port }),
    [ p ] ( boost::system::error_code const& ec, boost::asio::ip::tcp::resolver::iterator /*iterator*/) {
      if ( !ec) {
        p->setValue( true);
      } else {
        p->setException( std::runtime_error( "Unable to connect: " + ec.message()));
      }
    }
  );

  return p->getFuture();
}

//template <typename MessageType>
//folly::Future<bool>
//message_base<MessageType>::_async_accept() {
//  auto p = std::make_shared<folly::Promise<bool>>();
//
//  boost::asio::async_accept( _socket,
//    [ p ] ( boost::system::error_code const& ec, boost::asio::ip::tcp::resolver::iterator /*iterator*/) {
//      if ( !ec) {
//        p->setValue( true);
//      } else {
//        p->setException( std::runtime_error( "Unable to connect: " + ec.message()));
//      }
//    }
//  );
//
//  return p->getFuture();
//}

template <typename MessageType>
folly::Future<folly::Unit>
message_base<MessageType>::_async_write( buffer_type const& buffer) {
  auto p = std::make_shared<folly::Promise<folly::Unit>>();

  boost::asio::async_write( _socket,
    boost::asio::buffer( MessageType::format_message( buffer)),
    [ p ] ( boost::system::error_code const& ec, std::size_t /*bytes_transferred*/) {
      if ( !ec) {
        p->setValue();
      } else {
        p->setException( std::runtime_error( "Unable to write request"));
      }
    }
  );

  return p->getFuture();
}

template <typename MessageType>
folly::Future<std::vector<char>>
message_base<MessageType>::_async_read_header() {
  char msg_header [ MessageType::HeaderLength ];

  auto p = std::make_shared<folly::Promise<std::vector<char>>>();

  boost::asio::async_read( _socket,
    boost::asio::buffer( msg_header, MessageType::HeaderLength),
    [ p, msg_header ] ( boost::system::error_code const& ec, std::size_t /*bytes_transferred*/) {
      if ( !ec) {
        p->setValue( std::vector<char>( MessageType::parse_header( msg_header), '\0'));
      } else {
        p->setException( std::runtime_error( "Unable to read header"));
      }
    }
  );

  return p->getFuture();
}

template <typename MessageType>
folly::Future<typename MessageType::buffer_type>
message_base<MessageType>::_async_read_body( std::vector<char> & vec) {
  auto p = std::make_shared<folly::Promise<buffer_type>>();

  boost::asio::async_read( _socket,
    boost::asio::buffer( vec),
    [ p, &vec ] ( boost::system::error_code const& ec, std::size_t /*bytes_transferred*/) {
      if ( !ec) {
        p->setValue( buffer_type( vec.begin(), vec.end()));
      } else {
        p->setException( std::runtime_error( "Unable to read body"));
      }
    }
  );

  return p->getFuture();
}
