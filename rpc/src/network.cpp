#include "../detail/network.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
#include <utility>

RPC_BEGIN

tcp_server::tcp_server(uint16_t port)
    : acceptor_(io_context_, 
        ASIO::ip::tcp::endpoint(ASIO::ip::tcp::v4(), port)) {
}

tcp_server::~tcp_server() {
    stop();
}

void tcp_server::set_message_handler(message_handler handler) {
    request_handler_ = [handler = std::move(handler)](
        std::vector<uint8_t>&& data,
        connection_handle /*connection*/) {
        if (handler) {
            handler(data);
        }
    };
}

void tcp_server::set_request_handler(request_handler handler) {
    request_handler_ = std::move(handler);
}

void tcp_server::run() {
    do_accept();
    io_context_.run();
}

void tcp_server::stop() {
    io_context_.stop();
}

size_t tcp_server::active_connections() const {
    return sessions_.size();
}

void tcp_server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, ASIO::ip::tcp::socket socket) {
            if (!ec) {
                auto new_session = std::make_shared<session>(std::move(socket));
                
                //注意这里要使用weak_ptr,如果使用shared_ptr,容易出现 
                //session内部的回调函数里面引用了session自身造成循环引用
                
                std::weak_ptr<session> weak_session = new_session;
                connection_handle connection(weak_session);
                
                std::cout << "[Server] New connection from: " 
                          << new_session->remote_endpoint() << std::endl;
                
                new_session->start(
                    [this, connection](std::vector<uint8_t>&& data) {
                        if (!request_handler_) {
                            return;
                        }
                        // We exposure connection_handle to stub layer,
                        // when we add new function in stub layer in the
                        // future, we can avoid the detail of socket
                        request_handler_(std::move(data), connection);
                    },
                    [this](const boost::system::error_code& error) {
                        std::cout << "[Server] Session error: " << error.message() << std::endl;
                        cleanup_sessions();
                    }
                );
                
                sessions_.push_back(new_session);
            } else {
                std::cerr << "[Server] Accept error: " << ec.message() << std::endl;
            }
            
            do_accept();
        });
}

void tcp_server::cleanup_sessions() {
    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(),
            [](const std::shared_ptr<session>& s) {
                return !s->is_open();
            }),
        sessions_.end()
    );
}

////////////// tcp_client

tcp_client::tcp_client() {
}

tcp_client::~tcp_client() {
    disconnect();
}

bool tcp_client::connect(const std::string& host, uint16_t port) {
    try {
        ASIO::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        
        ASIO::ip::tcp::socket socket(io_context_);
        ASIO::connect(socket, endpoints);
        
        session_ = std::make_shared<session>(std::move(socket));
        
        session_->start(
            [this](std::vector<uint8_t>&& data) {
                {
                    std::lock_guard<std::mutex> lock(response_mutex_);
                    response_buffer_ = data;
                    response_received_ = true;
                }
                response_cv_.notify_one();
                
                if (message_handler_) {
                    message_handler_(data);
                }
            },
            [](const boost::system::error_code& ec) {
                std::cerr << "[Client] Session error: " << ec.message() << std::endl;
            }
        );
        
        std::thread([this]() {
            io_context_.run();
        }).detach();
        
        std::cout << "[Client] Connected to " << host << ":" << port << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Client] Connect failed: " << e.what() << std::endl;
        return false;
    }
}

void tcp_client::set_message_handler(message_handler handler) {
    message_handler_ = std::move(handler);
}

bool tcp_client::send_and_wait(const std::vector<uint8_t>& request,
                                std::vector<uint8_t>& response,
                                int timeout_ms) {
    if (!session_ || !session_->is_open()) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(response_mutex_);
        response_received_ = false;
        response_buffer_.clear();
    }
    
    session_->async_write(request);
    
    std::unique_lock<std::mutex> lock(response_mutex_);
    if (response_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this]() { return response_received_; })) {
        response = response_buffer_;
        return true;
    }
    return false;
}

void tcp_client::async_send(const std::vector<uint8_t>& data) {
    if (session_ && session_->is_open()) {
        session_->async_write(data);
    }
}

void tcp_client::disconnect() {
    if (session_) {
        session_->close();
        session_.reset();
    }
    stop();
}

bool tcp_client::is_connected() const {
    return session_ && session_->is_open();
}

void tcp_client::run() {
    io_context_.run();
}

void tcp_client::stop() {
    io_context_.stop();
}

RPC_END
