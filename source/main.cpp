#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include <PacketParser.hpp>
#include <PacketPrintHandler.hpp>
#include <TcpServerTemplate.hpp>

#include "Params.hpp"

int main(int argc, char* argv[]) {
    try {
        auto params = Params(argc, argv);
        if (params.no_run)
        {
            return 0;
        }

        boost::asio::io_context io_context;

        auto factory = [] { return std::make_unique<PacketParser<PacketPrintHandler>>(); };
        using FactoryType = decltype(factory);
        auto server = tcp_server::TcpServer<FactoryType, 256>(io_context, factory, params.port);

        std::cerr << "Server listening on port " << server.port() << '\n';

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&server](const boost::system::error_code&, int)
        {
            server.stop();
        });

        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
