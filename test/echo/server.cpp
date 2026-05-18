#include <detail/session.hpp>
#include <detail/network.hpp>
#include <iostream>
#include <string>
#include <utility>

using namespace RPC;

int main() {
	std::cout << "=== RPC Network Layer Test ===" << std::endl;

	tcp_server server(8888);

	server.set_request_handler([&server](std::vector<uint8_t>&& data, connection_handle connection) {
		std::string msg(data.begin(), data.end());

		if (!msg.substr(0, 4).compare("exit")) {
			connection.close();
			server.stop();
			return;
		}

		std::cout << "[Server] Received: " << msg << std::endl;
		connection.reply(std::move(data));
		});

	server.run();
	return 0;
}
