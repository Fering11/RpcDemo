#ifndef RPC_NETWORK_HEAD
#define RPC_NETWORK_HEAD
#include "predef.h"
#include <boost/asio.hpp>
#include "session.hpp"
#include <deque>
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <utility>

RPC_BEGIN

class tcp_server;
//A lightweight version of session
//Exposure session function to stub layer
class connection_handle {
public:
    connection_handle() = default;

    void reply(const std::vector<uint8_t>& data) const {
        if (auto current_session = session_.lock()) {
            current_session->async_write(data);
        }
    }

    void reply(std::vector<uint8_t>&& data) const {
        if (auto current_session = session_.lock()) {
            current_session->async_write(std::move(data));
        }
    }

    void close() const {
        if (auto current_session = session_.lock()) {
            current_session->close();
        }
    }

    bool is_open() const {
        if (auto current_session = session_.lock()) {
            return current_session->is_open();
        }
        return false;
    }

    std::string remote_endpoint() const {
        if (auto current_session = session_.lock()) {
            return current_session->remote_endpoint();
        }
        return "unknown";
    }

private:
    explicit connection_handle(std::weak_ptr<session> session)
        : session_(std::move(session)) {
    }

private:
    std::weak_ptr<session> session_;

    friend class tcp_server;
};

using request_handler = std::function<void(std::vector<uint8_t>&&, connection_handle)>;

/**
 * @brief TCP Server
 * 
 * Responsibilities:
 * - Listen on specified port
 * - Accept client connections
 * - Manage all active sessions
 * - Provide message handling interface
 */
class tcp_server {
public:
    /**
     * @brief Constructor
     * @param port Listening port
     */
    explicit tcp_server(uint16_t port);

    /**
     * @brief Destructor
     */
    ~tcp_server();

    /**
     * @brief Set message handler callback
     * @param handler Message handler function
     */
    void set_message_handler(message_handler handler);

    /**
     * @brief Set request handler callback
     * @param handler Request handler function. The connection handle controls the same session.
     */
    void set_request_handler(request_handler handler);

    /**
     * @brief Start server (blocking)
     */
    void run();

    /**
     * @brief Stop server
     */
    void stop();

    /**
     * @brief Get number of active connections
     */
    size_t active_connections() const;

private:
    // Start accepting new connections
    void do_accept();

    // Remove closed sessions
    void cleanup_sessions();

private:
    ASIO::io_context io_context_;
    ASIO::ip::tcp::acceptor acceptor_;
    
    // All active sessions
    std::deque<std::shared_ptr<session>> sessions_;
    
    // Request handler callback
    request_handler request_handler_;
};

/**
 * @brief TCP Client
 * 
 * Responsibilities:
 * - Connect to server
 * - Send and receive messages
 * - Manage single connection lifecycle
 */
class tcp_client {
public:
    /**
     * @brief Constructor
     */
    tcp_client();

    /**
     * @brief Destructor
     */
    ~tcp_client();

    /**
     * @brief Connect to server
     * @param host Server address
     * @param port Server port
     * @return Whether connection succeeded
     */
    bool connect(const std::string& host, uint16_t port);

    /**
     * @brief Set message handler callback
     * @param handler Message handler function
     */
    void set_message_handler(message_handler handler);

    /**
     * @brief Synchronously send message and wait for response
     * @param request Request message
     * @param response Response message (output parameter)
     * @param timeout_ms Timeout in milliseconds
     * @return Whether succeeded
     */
    bool send_and_wait(const std::vector<uint8_t>& request, 
                       std::vector<uint8_t>& response,
                       int timeout_ms = 5000);

    /**
     * @brief Asynchronously send message
     * @param data Message content
     */
    void async_send(const std::vector<uint8_t>& data);

    /**
     * @brief Disconnect
     */
    void disconnect();

    /**
     * @brief Check if connected
     */
    bool is_connected() const;

    /**
     * @brief Run io_context (for async operations)
     */
    void run();

    /**
     * @brief Stop io_context
     */
    void stop();

private:
    ASIO::io_context io_context_;
    std::shared_ptr<session> session_;
    
    // For synchronous response waiting
    std::vector<uint8_t> response_buffer_;
    bool response_received_ = false;
    std::mutex response_mutex_;
    std::condition_variable response_cv_;
    
    // Message handler callback
    message_handler message_handler_;
};

RPC_END

#endif
