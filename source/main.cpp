#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include <PacketParser.hpp>
#include <PacketPrintHandler.hpp>
#include <TcpServerTemplate.hpp>

int main()
{
    boost::asio::io_context io_context;

    auto factory = [] { return std::make_unique<PacketParser<PacketPrintHandler>>(); };
    using FactoryType = decltype(factory);
    auto server = tcp_server::TcpServer<FactoryType, 256>(io_context, factory, 0);

    std::cerr << "Server listening on port " << server.port() << '\n';

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&server](const boost::system::error_code&, int)
    {
        server.stop();
    });

    io_context.run();

    return 0;
}
