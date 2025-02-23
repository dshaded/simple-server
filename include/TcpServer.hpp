#pragma once

#include <array>
#include <boost/asio.hpp>
#include <concepts>
#include <span>

namespace tcp_server
{
    using boost::asio::ip::tcp;
    namespace ip = boost::asio::ip;
    namespace placeholders = boost::asio::placeholders;

    template <typename Fct>
    concept BufferHandlerFactory = requires(Fct f, std::span<char> data) {
        // { f() } -> std::same_as<std::unique_ptr<std::remove_pointer<std::invoke_result_t<Fct>>>>;
        { (*f())(data) } -> std::same_as<void>;
    };

    template <BufferHandlerFactory Factory, int receive_buffer_size>
    class TcpServer
    {
        tcp::acceptor acceptor_;
        Factory& factory_;
        using BufferHandlerType = typename std::remove_reference<decltype(*factory_())>::type;

    public:
        TcpServer(boost::asio::io_context& io_context, Factory& handlerFactory, ip::port_type port) :
            acceptor_{io_context, tcp::endpoint{tcp::v4(), port}}, factory_{handlerFactory}
        {
            do_accept();
        }

        void stop() { acceptor_.close(); }

        [[nodiscard]] ip::port_type port() const { return acceptor_.local_endpoint().port(); }

    private:
        void do_accept()
        {
            acceptor_.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket)
                {
                    if (acceptor_.is_open())
                    {
                        if (!ec)
                        {
                            std::make_shared<Session>(std::move(socket), std::move(factory_()))->start();
                        }
                        do_accept();
                    }
                });
        }

        struct Session : std::enable_shared_from_this<Session>
        {
            tcp::socket socket_;
            std::unique_ptr<BufferHandlerType> handler_;
            std::array<char, receive_buffer_size> buffer_;

            Session(tcp::socket&& socket, std::unique_ptr<BufferHandlerType> handler) :
                socket_{std::move(socket)}, handler_{std::move(handler)}, buffer_{}
            {
            }

            void start()
            {
                socket_.async_read_some(boost::asio::buffer(buffer_),
                                        std::bind(&Session::do_read, this->shared_from_this(), placeholders::error,
                                                  placeholders::bytes_transferred));
            }

            void do_read(boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    (*handler_)(std::span(buffer_.data(), length));
                    start();
                }
            }
        };
    };
} // namespace tcp_server
