// Demo.cpp: 定义应用程序的入口点。
//

#include "Demo.h"
#include <memory>
#include <boost/asio.hpp>
#include <iostream>

using namespace std;
namespace asio = boost::asio;
//通信管道
//状态机 control -> recv -> stop/echo -> do_write -> control
class Session {
	using tcp = asio::ip::tcp;
public:
	Session(tcp::socket&& socket):
		socket_(std::move(socket)),now_(echo) {
	}

	tcp::socket& sock() { return socket_; }
	
	void start() {
		//echo 
		control();
	}

	void control() {

		if (now_ == stop) {
			std::cout << "Session Exit\n";
			socket_.close();
			return;
		}
		//echo 服务器
		auto shd_buff = std::make_shared<std::string>(1024, 0);
		socket_.async_receive(asio::buffer(*shd_buff),
			[shd_buff,this](boost::system::error_code ec, size_t recv_len) {
				
				if (!shd_buff->substr(0,4).compare("exit")) {
					//如果输入的为exit
					now_ = stop;
					do_write("End.");
				} else {
					do_write(*shd_buff);
				}

			});

	}
private:
	void do_write(std::string buff) {
		
		auto heap_ptr = std::make_unique<std::string>(std::move(buff));
		auto ptr = heap_ptr->data();
		auto len = heap_ptr->length();

		socket_.async_write_some(asio::buffer(ptr,len),
			[hp = std::move(heap_ptr),this](boost::system::error_code ec, size_t write_len)mutable {
				//检测length是否发送完成
				if (hp->length() != write_len) {
					do_write(hp->substr(write_len));
				} else {
					//全部发送完成，不管ec
					control();
				}
				
				hp.release();

			});

	}
	
	enum State {
		echo, stop
	};

	State now_;
	tcp::socket socket_;
};

class SingleServer{
	using tcp = asio::ip::tcp;
public:
	SingleServer(asio::io_context& ctx, short port) :
		ctx_(ctx), acceptor_(ctx, tcp::endpoint(tcp::v4(), port), true),
		session_pool_(), idx_() {}

	void accept() {
		acceptor_.async_accept([this](boost::system::error_code ec,tcp::socket socket) {
			
			if (!ec) {
				std::cout << "Accept new session.\n";
				(session_pool_[idx_++] = std::make_shared<Session>(std::move(socket)))->start();

			}
			
			accept();

			});

	}

protected:
	int idx_;
	std::unordered_map<int, std::shared_ptr<Session>> session_pool_;
	tcp::acceptor acceptor_;
	asio::io_context& ctx_;
};


int main()
{
	cout << "Hello CMake." << endl;
	// 测试Boost ASIO
	try {
		asio::io_context io_context;
		SingleServer server(io_context, 13001);
		server.accept();
		io_context.run();
	}
	catch (std::exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}
	

	return 0;
}
