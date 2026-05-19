#ifndef RPC_CLIENT_STUB_HEAD
#define RPC_CLIENT_STUB_HEAD

#include "network.hpp"
#include "../method/operation.hpp"
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

RPC_BEGIN
//Coordinate the network layer and protobuf with business logic
//Most of code is in the traits.
class client_stub:public tcp_client {
public:
	template<class Method, class... Args>
	auto call(Method, Args&&... args)
		-> typename operation::rpc_method_traits<std::decay_t<Method>>::result_type {
		using traits = operation::rpc_method_traits<std::decay_t<Method>>;

		auto body = traits::make_request(std::forward<Args>(args)...).SerializeAsString();

		Request request;
		const auto request_id = next_request_id_.fetch_add(1);
		request.set_id(request_id);
		request.set_type(traits::id);
		request.set_body(std::move(body));

		auto request_payload = request.SerializeAsString();
		std::vector<uint8_t> request_bytes(request_payload.begin(), request_payload.end());
		std::vector<uint8_t> response_bytes;
		//Blocking, wait for 5 seconds
		if (!send_and_wait(request_bytes, response_bytes)) {
			throw std::runtime_error("rpc request timed out or connection is closed");
		}

		Response response;
		if (!response.ParseFromArray(response_bytes.data(), static_cast<int>(response_bytes.size()))) {
			throw std::runtime_error("failed to parse rpc response");
		}

		if (response.id() != request_id) {
			throw std::runtime_error("rpc response id does not match request id");
		}

		if (response.type() != traits::id) {
			throw std::runtime_error("rpc response method type does not match request type");
		}

		if (response.status() != STATUS_OK) {
			auto message = response.error_message();
			if (message.empty()) {
				message = "rpc request failed";
			}
			throw std::runtime_error(message);
		}

		return traits::parse_response(response);
	}

private:
	std::atomic<uint64_t> next_request_id_{1};
};


RPC_END


#endif
