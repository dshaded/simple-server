#include <iostream>
#include <string>
#include <boost/asio.hpp>

using std::string_literals::operator ""s;
using boost::asio::ip::tcp;

class session : public std::enable_shared_from_this<session>
{
public:
    // our sesseion holds the socket
    explicit session(tcp::socket socket)
        : m_socket(std::move(socket))
    {
    }

    // and run was already called in our server, where we just wait for requests
    void run()
    {
        wait_for_request();
    }

private:
    void wait_for_request()
    {
        // since we capture `this` in the callback, we need to call shared_from_this()
        auto self(shared_from_this());
        // and now call the lambda once data arrives
        // we read a string until the null termination character
        boost::asio::async_read_until(m_socket, m_buffer, "\0",
                                      [this, self](boost::system::error_code ec, std::size_t /*length*/)
                                      {
                                          // if there was no error, everything went well and for this demo
                                          // we print the data to stdout and wait for the next request
                                          if (!ec)
                                          {
                                              std::string data{
                                                  std::istreambuf_iterator<char>(&m_buffer),
                                                  std::istreambuf_iterator<char>()
                                              };
                                              // we just print the data, you can here call other api's
                                              // or whatever the server needs to do with the received data
                                              std::cout << data << std::endl;
                                              wait_for_request();
                                          }
                                          else
                                          {
                                              std::cout << "error: " << ec << std::endl;;
                                          }
                                      });
    }

private:
    tcp::socket m_socket;
    boost::asio::streambuf m_buffer;
};

class server
{
public:
    // we need an io_context and a port where we listen to
    server(boost::asio::io_context& io_context, short port)
        : m_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
    {
        // now we call do_accept() where we wait for clients
        do_accept();
    }

private:
    void do_accept()
    {
        // this is an async accept which means the lambda function is
        // executed, when a client connects
        m_acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                // let's see where we created our session
                std::cout << "creating session on: "
                    << socket.remote_endpoint().address().to_string()
                    << ":" << socket.remote_endpoint().port() << '\n';
                // create a session where we immediately call the run function
                // note: the socket is passed to the lambda here
                std::make_shared<session>(std::move(socket))->run();
            }
            else
            {
                std::cout << "error: " << ec.message() << std::endl;
            }
            // since we want multiple clients to connnect, wait for the next one by calling do_accept()
            do_accept();
        });
    }

private:
    tcp::acceptor m_acceptor;
};

int main()
{
    // here we create the io_context
    boost::asio::io_context io_context;
    // we'll just use an arbitrary port here
    server s(io_context, 25000);
    // and we run until our server is alive
    io_context.run();

    return 0;
}
