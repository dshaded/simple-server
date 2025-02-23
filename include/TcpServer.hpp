#pragma once

#include <boost/asio.hpp>

using boost::asio::io_context;

class TcpServer {
public:
    TcpServer(io_context& io_ctx, int port);
};