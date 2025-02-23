#pragma once

#include <array>
#include <boost/asio.hpp>
#include <concepts>
#include <span>

/**
 * This namespace contains only one directly usable class template - TcpServer.
 * All other members are primarily for internal use.
 */
namespace tcp_server
{
    // Just a few shortcuts.
    using boost::asio::ip::tcp;
    namespace ip = boost::asio::ip;
    namespace placeholders = boost::asio::placeholders;

    // An attempt to provide better type checking for the handler factory template parameter.
    template <typename Fct>
    concept BufferHandlerFactory = requires(Fct f, std::span<char> data) {
        // todo - find a proper concept syntax to check that the factory returns std::unique_ptr<BufferHandler>
        // { f() } -> std::same_as<std::unique_ptr<std::remove_pointer<std::invoke_result_t<Fct>>>>;
        { (*f())(data) } -> std::same_as<void>;
    };

    /**
     * Boost::asio based tcp server. Accepts incoming connections on the specified port (or on automatically assigned if
     * zero).
     *
     * Expects io_context.run() to be called after the servers construction as any other Boost::asio asynchronous user.
     *
     * Data bytes received from connections are forwarded to buffer handler objects (one handler per connection).
     * Buffer handlers must be provided by a factory object specified at server creation time.
     *
     * @tparam Factory A callable object that provides unique_ptrs to buffer handlers. A buffer handler is
     * another callable object that accepts a sequence of bytes received from the network in the form of
     * std::span<char>.
     * @tparam receive_buffer_size A size of receive buffer allocated for each new TCP session.
     */
    template <BufferHandlerFactory Factory, int receive_buffer_size>
    class TcpServer
    {
        tcp::acceptor acceptor_;
        Factory& factory_; // The factory returns unique_ptr<BufferHandlerType>
        using BufferHandlerType = typename std::remove_reference<decltype(*factory_())>::type;

    public:
        /**
         * @param io_context Boost::asio context
         * @param handlerFactory The factory object responsible for creating unique pointers to BufferHandlers used by
         * client connections.
         * @param port TCP port to listen on (0 for automatic selection)
         */
        TcpServer(boost::asio::io_context& io_context, Factory& handlerFactory, ip::port_type port) :
            acceptor_{io_context, tcp::endpoint{tcp::v4(), port}}, factory_{handlerFactory}
        {
            do_accept(); // start listening immediately after construction
        }

        /**
         * A method of gracefully stopping the server.
         */
        void stop() { acceptor_.close(); }

        /**
         * @return TCP port number used by the server.
         */
        [[nodiscard]] ip::port_type port() const { return acceptor_.local_endpoint().port(); }

    private:
        void do_accept()
        {
            // asynchronously wait for the incoming connection
            acceptor_.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket)
                {
                    // incoming connection attempt
                    if (acceptor_.is_open()) // do not try to wait further if the acceptor was stopped (it will hang)
                    {
                        if (!ec)
                        {
                            // The session owning shared_pointer will be saved in the socket context as long as
                            // the connection remains active.
                            // unique_ptr<BufferHandler> obtained from the factory is owned by the session.
                            std::make_shared<Session>(std::move(socket), std::move(factory_()))->start();
                        }
                        // wait for another connection (not a recursion, creates a new lambda)
                        do_accept();
                    }
                });
        }

        // Nested private class responsible for handling a single connection
        struct Session : std::enable_shared_from_this<Session>
        {
            tcp::socket socket_;
            std::unique_ptr<BufferHandlerType> handler_;
            std::array<char, receive_buffer_size> buffer_;

            Session(tcp::socket&& socket, std::unique_ptr<BufferHandlerType> handler) :
                socket_{std::move(socket)}, handler_{std::move(handler)}, buffer_{}
            {
            }

            // Start waiting for the data to be received.
            void start()
            {
                // Read any available data into the buffer_. Might be one TCP packet at a time.
                // A shared pointer to this will be saved in the completion token.
                socket_.async_read_some(boost::asio::buffer(buffer_),
                                        std::bind(&Session::do_read, this->shared_from_this(), placeholders::error,
                                                  placeholders::bytes_transferred));
            }

            // Some data was received into the buffer_ - pass it to the handler.
            void do_read(boost::system::error_code ec, std::size_t length)
            {
                (*handler_)(std::span(buffer_.data(), length));
                // if the connection is terminated, the completion token will be destroyed along with the only
                // remaining shared pointer to this session...
                if (!ec)
                {
                    start(); // ... and if it's still active another token will be created in the start call.
                }
            }
        };
    };
} // namespace tcp_server
