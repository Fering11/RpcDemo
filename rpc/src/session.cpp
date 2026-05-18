#include "../detail/session.hpp"
#include <iostream>

#include <utility>

RPC_BEGIN

session::session(ASIO::ip::tcp::socket socket)
    : socket_(std::move(socket)),
      strand_(socket.get_executor()) {
}

void session::start(raw_message_handler on_message, error_handler on_error) {
    on_message_ = std::move(on_message);
    on_error_ = std::move(on_error);
    //这里read采用的是head|body 交替读取，
    //并且一次性不读取那么多，将数据缓存在tcp栈中
    do_read_header();
}

void session::async_write(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> copy = data;
    async_write(std::move(copy));
}

void session::async_write(std::vector<uint8_t>&& data) {
    ASIO::post(strand_, [self = shared_from_this(),
        data = std::move(data)]() mutable {

        bool write_in_progress = !self->write_queue_.empty();
        self->write_queue_.push_back(std::move(data));
        
        if (!write_in_progress) {
            self->do_write();
        }
    });
}

void session::close() {
    ASIO::dispatch(strand_, [self = shared_from_this()]() {
        self->do_close();
    });
}

bool session::is_open() const {
    return socket_.is_open();
}

std::string session::remote_endpoint() const {
    try {
        auto ep = socket_.remote_endpoint();
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    } catch (...) {
        return "unknown";
    }
}

void session::do_read_header() {
    auto self = shared_from_this();
    
    // Read 4-byte message length
    read_buffer_.resize(4);
    
    ASIO::async_read(socket_, ASIO::buffer(read_buffer_),
        ASIO::bind_executor(strand_,
        [self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // Parse message length (big-endian)
                uint32_t body_length = 
                    (static_cast<uint32_t>(self->read_buffer_[0]) << 24) |
                    (static_cast<uint32_t>(self->read_buffer_[1]) << 16) |
                    (static_cast<uint32_t>(self->read_buffer_[2]) << 8) |
                    (static_cast<uint32_t>(self->read_buffer_[3]));
                
                // Check if message length is reasonable (max 10MB)
                if (body_length > 0 && body_length <= 10 * 1024 * 1024) {
                    self->do_read_body(body_length);
                } else {
                    // Invalid message length
                    self->do_close();
                    if (self->on_error_) {
                        self->on_error_(boost::system::errc::make_error_code(
                            boost::system::errc::message_size));
                    }
                }
            } else {
                // Read error
                self->do_close();
                if (self->on_error_) {
                    self->on_error_(ec);
                }
            }
        }));
}

void session::do_read_body(uint32_t body_length) {
    auto self = shared_from_this();
    
    // Prepare to receive message body
    read_buffer_.resize(body_length);
    
    ASIO::async_read(socket_, ASIO::buffer(read_buffer_),
        ASIO::bind_executor(strand_,
        [self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // Call message handler callback
                if (self->on_message_) {
                    self->on_message_(std::move(self->read_buffer_));
                }
                
                // Continue reading next message
                self->do_read_header();
            } else {
                // Read error
                self->do_close();
                if (self->on_error_) {
                    self->on_error_(ec);
                }
            }
        }));
}

void session::do_write() {
    if (write_queue_.empty()) {
        return;
    }
    
    auto self = shared_from_this();
    const auto& data = write_queue_.front();
    
    // Construct complete message: [4-byte length][message body]
    auto message = std::make_shared<std::vector<uint8_t>>();
    uint32_t body_length = static_cast<uint32_t>(data.size());
    
    // Add length header (big-endian)
    message->push_back(static_cast<uint8_t>((body_length >> 24) & 0xFF));
    message->push_back(static_cast<uint8_t>((body_length >> 16) & 0xFF));
    message->push_back(static_cast<uint8_t>((body_length >> 8) & 0xFF));
    message->push_back(static_cast<uint8_t>(body_length & 0xFF));
    
    // Add message body
    message->insert(message->end(), data.begin(), data.end());
    //TODO v0.1版本不优化，这里message是从堆里面分配，在后续版本中，改成queue，并且优化掉现在的write_queue_
    ASIO::async_write(socket_, ASIO::buffer(message->data(),message->size()),
        ASIO::bind_executor(strand_,
        [self, message](boost::system::error_code ec, std::size_t /*length*/) {
                //延长message的生命周期到此处
            if (!ec) {
                // Remove sent message
                self->write_queue_.pop_front();
                
                // Continue sending next message in queue
                if (!self->write_queue_.empty()) {
                    self->do_write();
                }
            } else {
                // Write error
                self->do_close();
                if (self->on_error_) {
                    self->on_error_(ec);
                }
            }
        }));
}

void session::do_close() {
    boost::system::error_code ignored;
    socket_.shutdown(ASIO::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

RPC_END
