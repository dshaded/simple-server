#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include <PacketParser.hpp>
#include <PacketPrinter.hpp>
#include <TcpServer.hpp>

#include "Params.hpp"

int main(int argc, char* argv[]) {
    try {
        const auto params = Params(argc, argv);
        if (params.no_run)
        {
            return params.invalid ? 1 : 0;
        }

        boost::asio::io_context io_context;

        auto printer = PacketPrinter(std::cout);
        auto factory = [&printer] { return std::make_unique<PacketParser<PacketPrinter>>(printer); };
        auto server = tcp_server::TcpServer<decltype(factory), 256>(io_context, factory, params.port);

        std::cerr << "Server listening on port " << server.port() << '\n';

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&server](const boost::system::error_code&, int)
        {
            server.stop();
        });

        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
