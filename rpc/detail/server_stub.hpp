#ifndef RPC_SERVER_STUB_HEAD
#define RPC_SERVER_STUB_HEAD

#include "network.hpp"
#include "../method/operation.hpp"
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

RPC_BEGIN

class server_stub:public tcp_server {

public:
	explicit server_stub(uint16_t port)
		: tcp_server(port) {
		set_request_handler([this](std::vector<uint8_t>&& data, connection_handle connection) {
			handle_request(std::move(data), connection);
		});
	}

private:
	void handle_request(std::vector<uint8_t>&& data, connection_handle connection) {
		Request request;
		Response response;

		if (!request.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
			response = make_error_response(
				0,
				MET_UNKNOW,
				STATUS_PARSE_ERROR,
				"failed to parse rpc request");
			connection.reply(serialize_response(response));
			return;
		}

		response = dispatch(request);
		connection.reply(serialize_response(response));
	}

	Response dispatch(const Request& request) {
		switch (request.type()) {
		case MET_ADD:
			return dispatch_one<operation::add_tag>(request);
		case MET_DIV:
			return dispatch_one<operation::div_tag>(request);
		case MET_CALC_MATRIX:
			return dispatch_one<operation::calc_matrix_tag>(request);
		default:
			return make_error_response(
				request.id(),
				request.type(),
				STATUS_METHOD_NOT_FOUND,
				"rpc method not found");
		}
	}

	template<class Method>
	Response dispatch_one(const Request& request) {
		using traits = operation::rpc_method_traits<Method>;

		typename traits::request_type args;
		if (!args.ParseFromString(request.body())) {
			return make_error_response(
				request.id(),
				request.type(),
				STATUS_PARSE_ERROR,
				"failed to parse rpc request body");
		}

		try {
			auto result = traits::invoke(args);

			Response response;
			response.set_id(request.id());
			response.set_type(traits::id);
			response.set_status(STATUS_OK);
			response.set_body(traits::serialize_result(result));
			return response;
		}
		catch (const std::exception& error) {
			return make_error_response(
				request.id(),
				request.type(),
				STATUS_INVOKE_ERROR,
				error.what());
		}
		catch (...) {
			return make_error_response(
				request.id(),
				request.type(),
				STATUS_INTERNAL_ERROR,
				"unknown rpc invoke error");
		}
	}

	static Response make_error_response(
		uint64_t id,
		MethodType type,
		StatusCode status,
		std::string message) {
		Response response;
		response.set_id(id);
		response.set_type(type);
		response.set_status(status);
		response.set_error_message(std::move(message));
		return response;
	}

	static std::vector<uint8_t> serialize_response(const Response& response) {
		auto payload = response.SerializeAsString();
		return std::vector<uint8_t>(payload.begin(), payload.end());
	}

};
RPC_END

#endif
