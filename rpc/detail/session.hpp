#ifndef RPC_SESSION_HEAD
#define RPC_SESSION_HEAD

#include "predef.h"
#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <vector>
#include <deque>
#include <cstdint>

RPC_BEGIN

// Message handler callback: called when a complete message is received
// Parameter: message content as byte array
using message_handler = std::function<void(const std::vector<uint8_t>&)>;

// Raw session handler: owns the received message bytes.
using raw_message_handler = std::function<void(std::vector<uint8_t>&&)>;

// Error handler callback: called when an error occurs
// Parameter: error code
using error_handler = std::function<void(const boost::system::error_code&)>;

/**
 * @brief Session class handles message read/write for a single TCP connection
 * 
 * Protocol format: [4-byte length (big-endian)][message body]
 * 
 * Features:
 * - Independent connection management
 * - Length-prefix based message boundary handling
 * - Asynchronous read/write
 * - Callback-driven
 * - Thread-safe through strand (supports multi-threaded io_context)
 */
class session : public std::enable_shared_from_this<session> {
public:
    /**
     * @brief construction
     * @param socket connected socket
     */
    session(ASIO::ip::tcp::socket socket);

    /**
     * @brief Start session and begin receiving messages
     * @param on_message Message handler callback
     * @param on_error Error handler callback
     */
    void start(raw_message_handler on_message, error_handler on_error);

    /**
     * @brief Asynchronously send a message
     * @param data Message content
     */
    void async_write(const std::vector<uint8_t>& data);

    /**
     * @brief Asynchronously send a message (move semantics)
     * @param data Message content
     */
    void async_write(std::vector<uint8_t>&& data);

    /**
     * @brief Close the connection
     */
    void close();

    /**
     * @brief Check if connection is open
     */
    bool is_open() const;

    /**
     * @brief Get remote endpoint information
     */
    std::string remote_endpoint() const;

private:
    // Asynchronously read message header (4-byte length)
    void do_read_header();

    // Asynchronously read message body
    void do_read_body(uint32_t body_length);

    // Process write queue
    void do_write();

    // Close socket from inside the strand.
    void do_close();

private:
    ASIO::ip::tcp::socket socket_;
    
    // Strand for thread-safe operation serialization
    ASIO::strand<ASIO::ip::tcp::socket::executor_type> strand_;
    // Read buffer
    std::vector<uint8_t> read_buffer_;
    uint32_t read_body_length_ = 0;

    // Write queue (supports concurrent writes)
    std::deque<std::vector<uint8_t>> write_queue_;

    // Callback functions
    raw_message_handler on_message_;
    error_handler on_error_;
};

RPC_END

#endif
