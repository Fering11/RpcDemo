#include <boost/asio.hpp>
#include <iostream>
#include <thread>

using boost::asio::ip::tcp;

class Client {
public:
    Client(boost::asio::io_context& io,
        const std::string& host,
        short port)
        : io_(io),
        socket_(io),
        buffer_(),
        resolver_(io) {

        auto endpoints = resolver_.resolve(host, std::to_string(port));

        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    std::cout << "Connected to server\n";
                    
                } else {
                    std::cout << "Connect failed: " << ec.message() << "\n";
                }
            });
    }

private:
    void do_read() {
        socket_.async_read_some(boost::asio::buffer(buffer_),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                } else {
                    socket_.close();
                }
            });
    }

    void do_write(const std::string& msg) {
        boost::asio::async_write(socket_,
            boost::asio::buffer(msg),
            [this](boost::system::error_code ec, std::size_t) {
                if (!ec) {

                } else {
                    socket_.close();
                }
            });
    }

private:

    boost::asio::io_context& io_;
    tcp::socket socket_;
    tcp::resolver resolver_;
    std::array<char, 4096> buffer_;
};

int main() {
    try {
        boost::asio::io_context io;

        Client client(io, "127.0.0.1", 13001);

        io.run();
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}