#pragma once

#include <string>
#include <vector>

#include <arpa/inet.h>

class admin_message
{
public:
  using buffer_type = std::string;
  static constexpr std::size_t HeaderLength = 16;

  static buffer_type format_message( buffer_type const& message) {
    std::vector<char> v( HeaderLength); // set v's size to HeaderLength so copy of message can work correctly
    v.reserve( message.length() + HeaderLength); // reserve space so no extra alloc's need to be made

    std::uint32_t * p_data = (std::uint32_t *) &v[ 0 ];

    //       std::tie( p_data[ 0 ], p_data[ 1 ]  , p_data[ 2 ] , p_data[ 3 ]              ) =
    //std::make_tuple( 0          , htonl( 0x01) , 0           , htonl( message.length()) );
    p_data[ 0 ] = 0;
    p_data[ 1 ] = htonl( 0x01);
    p_data[ 2 ] = 0;
    p_data[ 3 ] = htonl( message.length());

    std::copy( message.begin(), message.end(), std::back_inserter( v));
    return buffer_type( v.begin(), v.end());
  }

  static std::size_t parse_header( char const* header) {
    std::uint32_t const* p_data = (std::uint32_t const*) header;

    std::uint32_t protocol, body_length;

    //       std::tie( std::ignore , protocol            , std::ignore , body_length         ) =
    //std::make_tuple( p_data[ 0 ] , ntohl( p_data[ 1 ]) , p_data[ 2 ] , ntohl( p_data[ 3 ]) );

    protocol    = ntohl( p_data[ 1 ]);
    body_length = ntohl( p_data[ 3 ]);

    return body_length;
  }
};

