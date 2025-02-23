#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include <CommandPrinter.hpp>
#include <PacketParser.hpp>
#include <TcpServer.hpp>

#include "Params.hpp"

int main(int argc, char* argv[])
{
    try
    {
        // parse the command line params.
        const auto params = Params(argc, argv);
        if (params.no_run)
        {
            return params.invalid ? 1 : 0;
        }

        boost::asio::io_context io_context;

        // create tcp server -> packet parser factory -> command printer chain.
        auto printer = CommandPrinter(std::cout);
        auto factory = [&printer] { return std::make_unique<PacketParser<CommandPrinter>>(printer); };
        auto server = tcp_server::TcpServer<decltype(factory), 256>(io_context, factory, params.port);

        if (server.port() != params.port)
        {
            // need to tell which port we are using, but stdout is reserved for the data output, so use stderr.
            std::cerr << "Server listening on port " << server.port() << '\n';
        }

        // wait for ctrl-c or sigterm to stop the server.
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&server](const boost::system::error_code&, int) { server.stop(); });

        // begin asio event loop.
        io_context.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
