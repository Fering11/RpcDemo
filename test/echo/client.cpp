#include <detail/session.hpp>
#include <detail/network.hpp>
#include <iostream>
#include <string>
#include <utility>

using namespace RPC;

int main() {
	std::cout << "=== RPC Network Layer Test ===" << std::endl;

    tcp_client client;

    if (!client.connect("127.0.0.1", 8888)) {
        std::cerr << "[Client] Failed to connect" << std::endl;
        return 0;
    }

    while (client.is_connected()) {
        std::string buff;
        
        std::cout << "[C]>>> ";
        std::cin >> buff;

        std::vector<uint8_t> request(buff.begin(), buff.end());
        std::vector<uint8_t> response;
        if (client.send_and_wait(request, response, 5000)) {
            std::string resp_msg(response.begin(), response.end());
            std::cout << "[S]<<< " << resp_msg << std::endl;
        } else {
            std::cout << "[S] timeout" << std::endl;
        }
    }
    std::cout << "Disconnected ";
	return 0;
}