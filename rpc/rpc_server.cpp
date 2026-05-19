#include "server.hpp"
#include <iostream>

int main() {
	RPC::server server(8888);

	std::cout << "[Server] RPC server listening on 8888" << std::endl;
	server.run();

	return 0;
}
