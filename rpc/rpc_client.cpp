#include "client.hpp"
#include "method/operation.hpp"
#include <exception>
#include <iostream>

int main() {
	RPC::client client;

	if (!client.connect("127.0.0.1", 8888)) {
		std::cerr << "[Client] Failed to connect" << std::endl;
		return 1;
	}

	try {
		auto result = client.call(operation::add, 12, 21);
		std::cout << "[Client] add(12, 21) = " << result << std::endl;
	}
	catch (const std::exception& error) {
		std::cerr << "[Client] RPC failed: " << error.what() << std::endl;
		return 1;
	}

	return 0;
}
